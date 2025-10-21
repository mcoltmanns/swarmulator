//
// Created by moltmanns on 5/14/25.
//

#define RAYGUI_IMPLEMENTATION

#include <omp.h>
#include <string>
#include <H5Cpp.h>

#include "raygui.h"
#include "raylib.h"
#include "agent/Boid.h"
#include "sim/Simulation.h"
#include "sim/util.h"

int main(int argc, char** argv) {
    int init_agent_count = 10000;
    int window_w = 1080;
    int window_h = 720;
    constexpr Vector3 world_size = {150, 150, 150};
    constexpr int subdivisions = 20;
    float cam_speed = 1.f;
    omp_set_num_threads(omp_get_max_threads());
    float run_for = 60; // how many seconds to run the simulation for

    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-n")) {
        init_agent_count = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-w")) {
        window_w = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-h")) {
        window_h = std::stoi(o);
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-t")) {
        omp_set_num_threads(std::stoi(o));
    }
    if (const auto o = swarmulator::get_opt(argv, argv + argc, "-p")) {
        omp_set_num_threads(std::max(1, std::min(std::stoi(o), omp_get_max_threads()))); // number of threads used should not be greater than the maximum threads available on device
    }
    if (swarmulator::opt_exists(argv, argv + argc, "--vsync")) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    if (swarmulator::opt_exists(argv, argv + argc, "--aa")) {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
    }

    // seed rng
    auto s = time(nullptr);
    srand(s);
    std::cout << "Random seed: " << s << std::endl;

    // init window and camera
    InitWindow(window_w, window_h, "Swarmulator");
    Camera camera = {
        {2 * world_size.x, 2 * world_size.y, 2 * world_size.z},
        {0, 0, 0},
        {0, 1, 0},
        35.f,
        CAMERA_PERSPECTIVE,
    };

    auto simulation = swarmulator::Simulation(world_size, subdivisions);

    // add the boids
    std::string vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/boid.vert";
    const std::string fs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/simobject.frag";
    const Shader boid_shader = LoadShader(vs_src_path.c_str(), fs_src_path.c_str());
    const auto tri = std::vector<Vector3>{
            { -0.86, -0.5, 0.0 },
            { 0.86, -0.5, 0.0 },
            { 0.0f,  1.0f, 0.0f }
    };
    simulation.object_instancer_.new_group<swarmulator::Boid>(tri, boid_shader, "boid");

    // add some other demo objects
    vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/red.vert";
    const Shader stat_shader = LoadShader(vs_src_path.c_str(), fs_src_path.c_str());
    simulation.object_instancer_.new_group<swarmulator::SimObject>(tri, stat_shader, "test_stationary");

    // initialize all the agents
    for (int i = 0; i < init_agent_count; i++) {
        const auto p = Vector4{(swarmulator::randfloat() - 0.5f) * world_size.x, (swarmulator::randfloat() - 0.5f) * world_size.y, (swarmulator::randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{swarmulator::randfloat() - 0.5f, swarmulator::randfloat() - 0.5f,
                               swarmulator::randfloat() - 0.5f, 0};
        auto obj = swarmulator::Boid(swarmulator::xyz(p), swarmulator::xyz(r));
        simulation.object_instancer_.add_object(obj);
        //simulation.logger_.queue_new_object(obj.type_name(), obj.get_id());
        // FIXME above throws
    }

    // initialize some stationary objects
    for (int i = 0; i < 50; i++) {
        const auto p = Vector4{(swarmulator::randfloat() - 0.5f) * world_size.x, (swarmulator::randfloat() - 0.5f) * world_size.y, (swarmulator::randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{swarmulator::randfloat() - 0.5f, swarmulator::randfloat() - 0.5f,
                               swarmulator::randfloat() - 0.5f, 0};
        auto obj = swarmulator::SimObject(swarmulator::xyz(p), swarmulator::xyz(r));
        simulation.object_instancer_.add_object(obj);
        // simulation.logger_.queue_new_object(obj.type_name(), obj.get_id());
        // FIXME above throws
    }

    // set up tables in the logger
    // simobject
    auto dummy = swarmulator::SimObject();
    simulation.logger_.create_object_group(dummy.type_name(), dummy.log().size(), 1);
    //simulation.logger_.queue_log_object_data(dummy.type_name(), {dummy.get_interaction_radius()}, false);
    // boid
    auto boid = swarmulator::Boid();
    simulation.logger_.create_object_group(boid.type_name(), boid.log().size(), 1);

    auto start_time = GetTime();
    size_t frames = 0;
    while (!WindowShouldClose() && GetTime() - start_time < run_for) {
        const float dt = GetFrameTime(); // set to 1 for 1 update/frame (max speed), set to GetFrameTime() for realtime

        // INPUT
        PollInputEvents();
        // camera
        const auto cam_speed_factor = GetFrameTime();
        if (IsKeyDown(KEY_D)) CameraYaw(&camera, cam_speed * cam_speed_factor, true);
        if (IsKeyDown(KEY_A)) CameraYaw(&camera, -cam_speed * cam_speed_factor, true);
        if (IsKeyDown(KEY_W)) CameraPitch(&camera, -cam_speed * cam_speed_factor, true, true, false);
        if (IsKeyDown(KEY_S)) CameraPitch(&camera, cam_speed * cam_speed_factor, true, true, false);
        if (IsKeyDown(KEY_Q)) CameraMoveToTarget(&camera, cam_speed * cam_speed_factor * Vector3Distance(camera.position, camera.target));
        if (IsKeyDown(KEY_E)) CameraMoveToTarget(&camera, -cam_speed * cam_speed_factor * Vector3Distance(camera.position, camera.target));

        // COMPUTE
        simulation.update(dt);

        // DRAW
        BeginDrawing();
        ClearBackground(RAYWHITE);
        BeginMode3D(camera);
        Matrix view = GetCameraMatrix(camera);

        // objects
        simulation.draw_objects(view);

        // gui
        DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        EndMode3D();

        // debug info
        DrawFPS(0, 0);
        DrawText(TextFormat("%zu/%zu agents", simulation.object_instancer_.size(), 0), 0, 20, 18, DARKGREEN);
        DrawText(TextFormat("%zu threads", omp_get_max_threads()), 0, 40, 18, DARKGREEN);
        DrawText(TextFormat("%.0f sim time", simulation.get_total_time()), 0, 60, 18, DARKGREEN);

        EndDrawing();
        frames++;
    }
    double fps = static_cast<double>(frames) / (GetTime() - start_time);

    CloseWindow();
    std::cout << "Average FPS: " << fps << std::endl;
    return 0;
}