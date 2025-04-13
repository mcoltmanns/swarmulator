//
// Created by moltmanns on 4/2/25.
//

#define RAYGUI_IMPLEMENTATION

#include <cstdio>
#include <ctime>
#include <iostream>
#include <omp.h>
#include <string>

#include "raygui.h"
#include "raymath.h"
#include "rlgl.h"
#include "util/StaticGrid.h"
#include "util/util.h"
#include "v3ops.h"


int main(int argc, char** argv) {
    int agent_count = 4096;
    int window_w = 800;
    int window_h = 600;
    constexpr Vector3 world_size = {1.f, 1.f, 1.f};
    constexpr int subdivisions = 20; // choose such that size / count > agent sense diameter (10-20 are good numbers)
    float time_scale = 1.f;
    float cam_speed = 1.f;
    float agent_scale = 1.f;
    bool draw_bounds = true;
    omp_set_num_threads(16);

    if (const auto o = get_opt(argv, argv + argc, "-n")) {
        agent_count = std::stoi(o);
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
        CAMERA_PERSPECTIVE
    };

    // init space partitioning grid
    auto grid = swarmulator::util::StaticGrid(world_size, subdivisions);

    // init spheres
    auto objects = std::vector<swarmulator::env::Sphere *>();

    // init agents (shaders and mesh)
    // TODO: make these shaders position-independent! (probably a constexpr string somewhere is best)
    Shader agent_shader = LoadShader("shaders/agent.vert", "shaders/agent.frag");
    auto agent_vao = rlLoadVertexArray();
    rlEnableVertexArray(agent_vao);
    constexpr Vector3 mesh[] = {
        {-0.86, -0.5, 0},
        {0.86, -0.5, 0},
        {0, 1, 0}
    };
    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(mesh, sizeof(mesh), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlDisableVertexArray();
    // initialize all the agents
    std::vector<swarmulator::agent::Agent *> agents;
    agents.reserve(agent_count);
    // also initialize the position and rotation buffers that we will pass to the shaders
    auto positions = static_cast<Vector4*>(RL_CALLOC(agent_count, sizeof(Vector4)));
    auto rotations = static_cast<Vector4*>(RL_CALLOC(agent_count, sizeof(Vector4)));
    for (int i = 0; i < agent_count; i++) {
        const auto p = Vector4{(randfloat() - 0.5f) * world_size.x, (randfloat() - 0.5f) * world_size.y, (randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
        agents.push_back(new swarmulator::agent::Agent(xyz(p), xyz(r)));
        positions[i] = p;
        rotations[i] = r;
    }
    auto pos_ssbo = rlLoadShaderBuffer(agent_count * sizeof(Vector4), positions, RL_DYNAMIC_COPY);
    auto rot_ssbo = rlLoadShaderBuffer(agent_count * sizeof(Vector4), rotations, RL_DYNAMIC_COPY);

    while (!WindowShouldClose()) {
        const float dt = GetFrameTime() * time_scale;
        // INPUT
        PollInputEvents();
        if (IsKeyDown(KEY_SPACE)) draw_bounds = !draw_bounds;
        // camera
        const auto cam_speed_factor = time_scale == 0 ? GetFrameTime() : (dt / time_scale);
        if (IsKeyDown(KEY_D)) CameraYaw(&camera, cam_speed * cam_speed_factor, true);
        if (IsKeyDown(KEY_A)) CameraYaw(&camera, -cam_speed * cam_speed_factor, true);
        if (IsKeyDown(KEY_W)) CameraPitch(&camera, -cam_speed * cam_speed_factor, true, true, false);
        if (IsKeyDown(KEY_S)) CameraPitch(&camera, cam_speed * cam_speed_factor, true, true, false);
        if (IsKeyDown(KEY_Q)) CameraMoveToTarget(&camera, cam_speed * cam_speed_factor);
        if (IsKeyDown(KEY_E)) CameraMoveToTarget(&camera, -cam_speed * cam_speed_factor);

        // COMPUTE
        // partition agents in their spaces
        grid.sort_agents(agents);
        // update all the agents
#pragma omp parallel for
        for (int i = 0; i < agent_count; i++) {
            auto agent = agents[i];
            // if oob, bounce
            if (agent->get_position().x <= -world_size.x / 2. || agent->get_position().x >= world_size.x / 2.
                || agent->get_position().y <= -world_size.y / 2. || agent->get_position().y >= world_size.y / 2.
                || agent->get_position().z <= -world_size.z / 2. || agent->get_position().z >= world_size.z / 2.) {
                agent->set_direction(agent->get_direction() - 0.5f * agent->get_position());
            }
            auto neighborhood = grid.get_neighborhood(*agent);
            agent->update(*neighborhood, objects, dt);
            positions[i].x = agent->get_position().x;
            positions[i].y = agent->get_position().y;
            positions[i].z = agent->get_position().z;
            rotations[i].x = agent->get_direction().x;
            rotations[i].y = agent->get_direction().y;
            rotations[i].z = agent->get_direction().z;
        }
        rlUpdateShaderBuffer(pos_ssbo, positions, agent_count * sizeof(Vector4), 0);
        rlUpdateShaderBuffer(rot_ssbo, rotations, agent_count * sizeof(Vector4), 0);

        // DRAW
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera);
        Matrix projection = rlGetMatrixProjection();
        Matrix view = GetCameraMatrix(camera);
        // agents
        rlEnableShader(agent_shader.id);
        SetShaderValueMatrix(agent_shader, 0, projection);
        SetShaderValueMatrix(agent_shader, 1, view);
        SetShaderValue(agent_shader, 2, &agent_scale, SHADER_UNIFORM_FLOAT);
        // send agents to draw shader
        rlBindShaderBuffer(pos_ssbo, 0);
        rlBindShaderBuffer(rot_ssbo, 1);
        // instanced agent draw
        rlEnableVertexArray(agent_vao);
        rlDrawVertexArrayInstanced(0, 3, agent_count);
        rlDisableVertexArray();
        rlDisableShader();
        rlCheckErrors();

        // gui
        if (draw_bounds) {
            DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        }
        EndMode3D();
        if (draw_bounds) {
            GuiSlider((Rectangle){static_cast<float>(window_w) - 250, 10, 200, 10}, "Time scale", TextFormat("%.5f", time_scale), &time_scale, 0, 10);
        }
        DrawFPS(0, 0);
        DrawText(TextFormat("%zu agents", agent_count), 0, 20, 18, DARKGREEN);
        DrawText(TextFormat("%zu threads", omp_get_max_threads()), 0, 40, 18, DARKGREEN);

        EndDrawing();
    }

    CloseWindow();
    for (const auto &agent : agents) {
        delete agent;
    }
    RL_FREE(positions);
    RL_FREE(rotations);
    return 0;
}
