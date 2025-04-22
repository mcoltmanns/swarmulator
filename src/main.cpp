//
// Created by moltmanns on 4/2/25.
//

#define RAYGUI_IMPLEMENTATION

#include <cstdio>
#include <ctime>
#include <iostream>
#include <omp.h>
#include <cstdint>
#include <string>
#include <list>

#include "raygui.h"
#include "rlgl.h"
#include "raymath.h"
#include "util/StaticGrid.h"
#include "util/util.h"
#include "v3ops.h"
#include "agent/NeuralAgent.h"


int main(int argc, char** argv) {
    int init_agent_count = 4096;
    int init_sphere_count = 100;
    int window_w = 800;
    int window_h = 600;
    int max_agents = 25000; // we can go higher, but this provides nice performance
    constexpr Vector3 world_size = {1.f, 1.f, 1.f};
    constexpr int subdivisions = 20; // choose such that size / count > agent sense diameter (10-20 are good numbers)
    float time_scale = 1.f;
    float cam_speed = 1.f;
    float agent_scale = 1.f;
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
    if (const auto o = opt_exists(argv, argv + argc, "--vsync")) {
        SetConfigFlags(FLAG_VSYNC_HINT);
    }
    if (const auto o = opt_exists(argv, argv + argc, "--aa")) {
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
        CAMERA_PERSPECTIVE
    };

    // init space partitioning grid
    auto grid = swarmulator::util::StaticGrid(world_size, subdivisions);

    // init spheres
    auto objects = std::vector<std::shared_ptr<swarmulator::env::Sphere>>();
    objects.reserve(init_sphere_count);

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
    std::list<std::shared_ptr<swarmulator::agent::NeuralAgent>> agents;
    // also initialize the agent data buffer we will pass to the shaders
    auto agents_data = static_cast<swarmulator::agent::SSBOAgent*>(RL_CALLOC(max_agents, sizeof(swarmulator::agent::SSBOAgent)));
    for (int i = 0; i < init_agent_count; i++) {
        const auto p = Vector4{(randfloat() - 0.5f) * world_size.x, (randfloat() - 0.5f) * world_size.y, (randfloat() - 0.5f) * world_size.z, 0};
        const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
        agents.push_back(std::make_shared<swarmulator::agent::NeuralAgent>(xyz(p), xyz(r)));
        agents.back()->to_ssbo(&agents_data[i]);
    }
    auto agents_ssbo = rlLoadShaderBuffer(max_agents * sizeof(swarmulator::agent::SSBOAgent), agents_data, RL_DYNAMIC_COPY);
    // initialize the spheres
    for (int i = 0; i < init_sphere_count; i++) {
        const auto p = Vector4{(randfloat() - 0.5f) * world_size.x, (randfloat() - 0.5f) * world_size.y, (randfloat() - 0.5f) * world_size.z, 0};
        objects.push_back(std::make_shared<swarmulator::env::Sphere>(xyz(p), 0.01, GREEN));
    }

    uint_fast64_t frames = 0;
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
        // so long as there are agents
        if (!agents.empty()) {
            // first remove the dead agents
            auto it = agents.begin();
            while (it != agents.end()) {
                if (!it.operator*()->is_alive()) {
                    it = agents.erase(it);
                }
                else {
                    ++it;
                }
            }
            // partition agents in their spaces
            grid.sort_agents(agents);
            // update all the agents
            // we can't fully parallelize a list, but iteration is very cheap. the update is what's expensive
            // so we use this structure to have every thread iterate, but only a single thread will access each element (single) without waiting for other threads (nowait)
            // however the index variable i is shared so that agents are copied into the right place in the shaderbuffer
            size_t i = 0;
#pragma omp parallel private(it) shared(i)
            {
                for (it = agents.begin(); it != agents.end(); ++it) {
#pragma omp single nowait
                    {
                        // if oob, bounce
                        if ((*it)->get_position().x <= -world_size.x / 2. || (*it)->get_position().x >= world_size.x / 2.
                            || (*it)->get_position().y <= -world_size.y / 2. || (*it)->get_position().y >= world_size.y / 2.
                            || (*it)->get_position().z <= -world_size.z / 2. || (*it)->get_position().z >= world_size.z / 2.) {
                                (*it)->set_direction((*it)->get_direction() - 0.5f * (*it)->get_position());
                            }
                        auto neighborhood = grid.get_neighborhood(**it);
                        (*it)->update(*neighborhood, objects, dt);
                        (*it)->to_ssbo(&agents_data[i++]);
                    }
                }
            }
            // update the draw shader with agent info
            rlUpdateShaderBuffer(agents_ssbo, agents_data, agents.size() * sizeof(swarmulator::agent::SSBOAgent), 0);
        }

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
        const auto size = agents.size(); // tell the shader how many agents it needs to draw
        SetShaderValue(agent_shader, 3, &size, SHADER_UNIFORM_INT);
        // send agents to draw shader
        rlBindShaderBuffer(agents_ssbo, 0);
        // instanced agent draw
        rlEnableVertexArray(agent_vao);
        rlDrawVertexArrayInstanced(0, 3, static_cast<int>(agents.size()));
        rlDisableVertexArray();
        rlDisableShader();
        // spheres
        for (const auto &obj : objects) {
            obj->draw(); // this cannot happen in parallel (raylib freaks out)
        }
        rlCheckErrors();

        // gui
        if (draw_bounds) {
            DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        }
        EndMode3D();
        if (draw_bounds) {
            GuiSlider((Rectangle){static_cast<float>(window_w) - 250, 10, 200, 10}, "Time scale", TextFormat("%.5f", time_scale), &time_scale, 0, 10);
        }
        // debug info
        DrawFPS(0, 0);
        DrawText(TextFormat("%zu/%zu agents", agents.size(), max_agents), 0, 20, 18, DARKGREEN);
        DrawText(TextFormat("%zu threads", omp_get_max_threads()), 0, 40, 18, DARKGREEN);
        DrawText(TextFormat("%zu iterations", frames++), 0, 60, 18, DARKGREEN);

        EndDrawing();
    }

    CloseWindow();
    RL_FREE(agents_data);
    UnloadShader(agent_shader);
    return 0;
}
