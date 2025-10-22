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

    Simulation::Simulation(const size_t win_w, const size_t win_h, const Vector3 world_size, const size_t grid_divisions) :
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

    void Simulation::update(const float dt, const bool log) {
        total_time_ += dt;
        ++total_steps_;

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

        // sort everything (don't seem to be issues here)
        grid_.sort_objects(object_instancer_);
        // begin a logging frame and log dynamic sim attributes if applicable
        if (log) {
            logger_.queue_begin_frame(total_time_);
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
                        object->update(neighborhood, dt);
                        if (log) {
                            logger_.queue_log_object_data(object->type_name(), object->log(), true);
                        }
                    }
                }
            }
        }

        // update gpu
        object_instancer_.update_gpu();

        // next logging frame
        if (log) {
            logger_.queue_advance_frame();
        }
    }

    void Simulation::run() {
        // everything is initialized, so we can run right into the main loop
        while (!WindowShouldClose() && (total_time_ < run_for_ || run_for_ == 0)) {
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

            // update
            update(dt, logger_.initialized());

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
            DrawText(TextFormat("%.0f sim time", total_time_), 0, 60, 18, DARKGREEN);
            DrawText(TextFormat("%zu updates", total_steps_), 0, 80, 18, DARKGREEN);
            EndDrawing();

            total_steps_++;
            total_time_ += dt;
        }

        double fps = static_cast<double>(total_steps_) / total_time_;

        CloseWindow();
        std::cout << "Average FPS: " << fps << std::endl;
    }
} // swarmulator