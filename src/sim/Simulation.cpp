//
// Created by moltmanns on 9/24/25.
//

#include "Simulation.h"

#include <omp.h>

namespace swarmulator {
    Simulation::Simulation() : grid_(world_size_, grid_divisions_), logger_() {
        InitWindow(800, 600, "Swarmulator");
        camera_ = {
            2 * world_size_,
            Vector3Zeros,
            Vector3UnitY,
            35,
            CAMERA_PERSPECTIVE
        };
        sim_threads_ = omp_get_max_threads();
        omp_set_num_threads(sim_threads_); // can use maximum threads if not logging
    }

    Simulation::Simulation(const int win_w, const int win_h, const Vector3 world_size, const int grid_divisions) :
        world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions), logger_() {
        InitWindow(win_w, win_h, "Swarmulator");
        camera_ = {
            2 * world_size_,
            Vector3Zeros,
            Vector3UnitY,
            35,
            CAMERA_PERSPECTIVE
        };
        sim_threads_ = omp_get_max_threads();
        omp_set_num_threads(sim_threads_); // can use maximum threads if not logging
    }

     Simulation::Simulation(const int win_w, const int win_h, const Vector3 world_size, const int grid_divisions, const float timestep, const size_t max_updates, const std::string &logfile_path, const int log_compression, const int log_interval) : world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions), logger_() {
        InitWindow(win_w, win_h, "Swarmulator");
        camera_ = {
            2 * world_size_,
            Vector3Zeros,
            Vector3UnitY,
            35,
            CAMERA_PERSPECTIVE
        };
        sim_threads_ = omp_get_max_threads() - 1; // use max threads -1 so the logger has a core
        omp_set_num_threads(sim_threads_);

        time_step_ = timestep;
        run_for_ = max_updates;

        // save logger variables
        // logger setup happens at top of any method which might use the logger if the logger hasn't been initialized yet
        // can't call it here because it makes reference to virtual functions, which are selected at compile-time
        logging_interval_ = log_interval;
        logfile_path_ = logfile_path;
        logger_deflate_ = log_compression;
    }


    void Simulation::update(const float dt, const bool log) {
        // sort everything (don't seem to be issues here)
        grid_.sort_objects(object_instancer_);
        // begin a logging frame and log dynamic sim attributes if applicable
        if (log) {
            logger_.queue_begin_frame(sim_time_);
            logger_.queue_log_sim_data(log_dynamic(), true);
        }

        // update everyone
        // iteration is cheap, processing is expensive
        // so have every thread iterate over the whole list
        // but only a single thread will access a specific element (single) while the others continue on (nowait)
        // this has no noticeable performance impact vs parallel vector access
        // worth parallelizing the outer loop? not unless number of object groups is large.
        for (auto grp = object_instancer_.begin(); grp != object_instancer_.end(); ++grp) {
            // update objects
            std::list<SimObject*>::iterator it;
#pragma omp parallel private(it) shared(grp, dt, log) default(none)
            {
                for (it = grp->second.objects.begin(); it != grp->second.objects.end(); ++it) {
#pragma omp single nowait
                    {
                        const auto object = *it;
                        auto neighborhood = grid_.get_neighborhood(object);
                        object->update(*this, neighborhood, dt);
                        if (log) {
                            logger_.queue_log_object_data(object->type_name(), object->log(), true);
                        }
                    }
                }
            }
        }

        // remove inactive objects, wrap bounds (donut world)
        for (auto group_it = object_instancer_.begin(); group_it != object_instancer_.end(); ++group_it) {
            auto obj_it = group_it->second.objects.begin();
            while (obj_it != group_it->second.objects.end()) {
                if (const auto obj_ptr = *obj_it; !obj_ptr->active()) {
                    obj_it = object_instancer_.remove_object(group_it, obj_it);
                }
                else {
                    obj_ptr->set_position(wrap_position(obj_ptr->get_position(), world_size_));
                    ++obj_it;
                }
            }
        }

        // update gpu
        object_instancer_.update_gpu();

        // next logging frame
        if (log) {
            logger_.queue_advance_frame();
        }

        ++total_steps_;
        sim_time_ += dt;
    }

    void Simulation::run() {
        initialize_logger_if_not_init();

        const double start = GetTime();
        // everything is initialized, so we can run right into the main loop
        while (!WindowShouldClose() && (total_steps_ < run_for_ || run_for_ == 0)) {
            const float dt = time_step_ == 0 ? GetFrameTime() : time_step_;

            // get input
            PollInputEvents();

            // move the camera
            const auto cam_speed_factor = GetFrameTime(); // camera always moves relative to real time
            if (IsKeyDown(KEY_D)) CameraYaw(&camera_, cam_speed_factor, true);
            if (IsKeyDown(KEY_A)) CameraYaw(&camera_, -cam_speed_factor, true);
            if (IsKeyDown(KEY_W)) CameraPitch(&camera_, -cam_speed_factor, true, true, false);
            if (IsKeyDown(KEY_S)) CameraPitch(&camera_, cam_speed_factor, true, true, false);
            if (IsKeyDown(KEY_Q)) CameraMoveToTarget(&camera_, cam_speed_factor * Vector3Distance(camera_.position, camera_.target));
            if (IsKeyDown(KEY_E)) CameraMoveToTarget(&camera_, -cam_speed_factor * Vector3Distance(camera_.position, camera_.target));
            // speed up/slow down time if running in fixed time mode
            if (run_for_ != 0) {
                if (IsKeyDown(KEY_EQUAL)) time_step_ += 0.001;
                if (IsKeyDown(KEY_MINUS)) time_step_ = std::max(time_step_ - 0.001f, 0.001f);
                if (IsKeyDown(KEY_ONE)) time_step_ = 1;
                if (IsKeyDown(KEY_ZERO)) time_step_ = 0.01;
            }

            // update
            // only log if we're in a logging interval and the logger is enabled
            update(dt, logger_.initialized() && total_steps_ % logging_interval_ == 0);

            // draw
            BeginDrawing();
            ClearBackground(RAYWHITE);
            BeginMode3D(camera_);
            Matrix view = GetCameraMatrix(camera_);
            // simobjects
            object_instancer_.draw_all(view);
            // ui
            DrawCubeWiresV(Vector3(0, 0, 0), world_size_, DARKGRAY);
            EndMode3D();
            DrawFPS(0, 0);
            DrawText(TextFormat("%zu objects", object_instancer_.size()), 0, 20, 18, DARKGREEN);
            DrawText(TextFormat("%zu threads", sim_threads_), 0, 40, 18, DARKGREEN);
            DrawText(TextFormat("%zu updates", total_steps_), 0, 60, 18, DARKGREEN);
            DrawText(TextFormat("%.2f sim time", sim_time_), 0, 80, 18, DARKGREEN);
            DrawText(TextFormat("%zu log tasks", logger_.tasks_queued()), 0, 100, 18, DARKGREEN);
            DrawText(TextFormat("%.3f dt", dt), 0, 120, 18, DARKGREEN);
            if (run_for_ != 0) {
                DrawText(TextFormat("%.2f%%", 100.f * (static_cast<float>(total_steps_) / static_cast<float>(run_for_))), 0, 140, 18, DARKGREEN);
            }
            EndDrawing();
        }

        const double fps = static_cast<double>(total_steps_) / (GetTime() - start);
        std::cout << "Average FPS: " << fps << std::endl;
        CloseWindow();
    }
} // swarmulator