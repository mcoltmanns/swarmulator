//
// Created by moltmanns on 5/5/25.
//

#define RAYGUI_IMPLEMENTATION

#include <ctime>
#include <iostream>
#include <omp.h>
#include <string>

#include "raygui.h"
#include "raylib.h"
#include "rlgl.h"
#include "Simulation.h"
#include "agent/PDAgent.h"
#include "util/util.h"

int main(int argc, char** argv) {
    int init_agent_count = 1000;
    int window_w = 800;
    int window_h = 600;
    constexpr Vector3 world_size = {100, 100, 100};
    constexpr int subdivisions = 10;
    float cam_speed = 1.f;
    float time_scale = 1.f;
    bool draw_bounds = true;
    omp_set_num_threads(16);

    if (const auto o = get_opt(argv, argv + argc, "-n")) {
        init_agent_count = std::stoi(o);
    }
    if (const auto o = get_opt(argv, argv + argc, "-w")) {
        window_w = std::stoi(o);
    }
    if (const auto o = get_opt(argv, argv + argc, "-h")) {
        window_h = std::stoi(o);
    }
    if (const auto o = get_opt(argv, argv + argc, "-t")) {
        omp_set_num_threads(std::stoi(o));
    }
    if (opt_exists(argv, argv + argc, "--vsync")) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    if (opt_exists(argv, argv + argc, "-aa")) {
        SetConfigFlags(FLAG_MSAA_4X_HINT);
    }

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

    auto simulation = Simulation<swarmulator::agent::PDAgent>(world_size, subdivisions);

    // init agents (shaders and mesh)
    // TODO: make these shaders position-independent! (probably a constexpr string somewhere is best)
    Shader agent_shader = LoadShader("/home/moltmanns/Documents/swarmulator/shaders/pd.vert", "/home/moltmanns/Documents/swarmulator/shaders/agent.frag");
    auto agent_vao = rlLoadVertexArray();
    rlEnableVertexArray(agent_vao);
    constexpr Vector3 mesh[] = {
        {-0.86, -0.5, 0},
        {0.86, -0.5, 0},
        {0, 1, 0},
    };
    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(mesh, sizeof(mesh), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlDisableVertexArray();
    // initialize all the agents
    for (int i = 0; i < init_agent_count; i++) {
        const auto p = Vector4{(randfloat() - 0.5f) * world_size.x, (randfloat() - 0.5f) * world_size.y, (randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
        auto a = std::make_shared<swarmulator::agent::PDAgent>(xyz(p), xyz(r));
        a->mutate();
        simulation.add_agent(a);
    }

    uint_fast64_t frames = 0;
    while (!WindowShouldClose()) {
        const float dt = GetFrameTime() * time_scale;
        // INPUT
        PollInputEvents();
        if (IsKeyDown(KEY_SPACE)) draw_bounds = !draw_bounds;
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
        // agents
        rlEnableShader(agent_shader.id);
        SetShaderValueMatrix(agent_shader, 0, projection);
        SetShaderValueMatrix(agent_shader, 1, view);
        SetShaderValue(agent_shader, 2, &swarmulator::agent::scale, SHADER_UNIFORM_FLOAT);
        const auto num_agents = simulation.get_agents_count(); // tell the shader how many agents it needs to draw
        SetShaderValue(agent_shader, 3, &num_agents, SHADER_UNIFORM_INT);
        // send agents to draw shader
        rlBindShaderBuffer(simulation.get_agents_ssbo(), 0);
        // instanced agent draw
        rlEnableVertexArray(agent_vao);
        rlDrawVertexArrayInstanced(0, 3, static_cast<int>(simulation.get_agents_count()));
        rlDisableVertexArray();
        rlDisableShader();
        // spheres
        simulation.draw_objects();
        rlCheckErrors();

        // gui
        if (draw_bounds) {
            DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        }
        EndMode3D();
        if (draw_bounds) {
            GuiSlider((Rectangle){static_cast<float>(window_w) - 250, 10, 200, 10}, "Time scale", TextFormat("%.5f", time_scale), &time_scale, 0, 100);
        }
        // debug info
        DrawFPS(0, 0);
        DrawText(TextFormat("%zu/%zu agents", simulation.get_agents_count(), simulation.get_max_agents()), 0, 20, 18, DARKGREEN);
        DrawText(TextFormat("%zu threads", omp_get_max_threads()), 0, 40, 18, DARKGREEN);
        DrawText(TextFormat("%zu iterations", frames++), 0, 60, 18, DARKGREEN);
        DrawText(TextFormat("%.0f seconds", GetTime()), 0, 80, 18, DARKGREEN);
        DrawText(TextFormat("%zu agents have existed", simulation.get_total_agents()), 0, 100, 18, DARKGREEN);

        EndDrawing();
    }

    CloseWindow();
    UnloadShader(agent_shader);
    return 0;
}
