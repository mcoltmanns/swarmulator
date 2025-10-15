//
// Created by moltmanns on 5/14/25.
//

#define RAYGUI_IMPLEMENTATION

#include <ctime>
#include <iostream>
#include <omp.h>
#include <string>

#include "raygui.h"
#include "raylib.h"
#include "rlgl.h"
#include "agent/Boid.h"
#include "sim/Simulation.h"
#include "sim/util.h"

int main(int argc, char** argv) {
    int init_agent_count = 5000;
    int window_w = 800;
    int window_h = 600;
    constexpr Vector3 world_size = {100, 100, 100};
    constexpr int subdivisions = 10;
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

    // init agents (shaders and mesh)
    // this is majorly ugly and sort of janky, but lets us compile the shader source in with the executable, instead of loading at runtime
    const std::string vs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/boid.vert";
    const std::string fs_src_path = "/home/moltma/Documents/swarmulator/src/shaders/agent.frag";
    const auto mesh = GenMeshCube(1, 1, 1);
    simulation.alloc_object_type<swarmulator::Boid>(vs_src_path, fs_src_path, mesh);
    // initialize all the agents
    for (int i = 0; i < init_agent_count; i++) {
        const auto p = Vector4{(swarmulator::randfloat() - 0.5f) * world_size.x, (swarmulator::randfloat() - 0.5f) * world_size.y, (swarmulator::randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{swarmulator::randfloat() - 0.5f, swarmulator::randfloat() - 0.5f,
                               swarmulator::randfloat() - 0.5f, 0};
        auto obj = std::make_shared<swarmulator::Boid>(swarmulator::xyz(p), swarmulator::xyz(r));
        simulation.add_object(obj);
    }

    /*simulation.set_min_agents(init_agent_count);
    simulation.set_max_agents(init_agent_count);
    simulation.logging_enabled_ = false;*/

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
        ClearBackground(WHITE);
        BeginMode3D(camera);
        Matrix projection = rlGetMatrixProjection();
        Matrix view = GetCameraMatrix(camera);

        // objects
        simulation.draw_objects(projection, view);

        // gui
        DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        EndMode3D();

        // debug info
        DrawFPS(0, 0);
        DrawText(TextFormat("%zu/%zu agents", simulation.get_objects_count(), 0), 0, 20, 18, DARKGREEN);
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