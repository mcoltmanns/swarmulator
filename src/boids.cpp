//
// Created by moltmanns on 3/28/25.
//

#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <ctime>
#include <iostream>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "util.h"

int num_boids = 32 * 1024;
float boid_sense_radius = 0.03f;
float boid_speed = 0.06f;
float rotation_speed = 1.f;
float cohesion_wt = 1.f;
float avoidance_wt = 1.f;
float alignment_wt = 1.f;

float cam_speed = 1.f;
float time_scale = 1.f;

int width = 800;
int height = 600;

bool draw_bounds = true;

Vector3 world_size = { 1.f, 1.f, 1.f };
Vector3 cell_count = { 1.f, 1.f, 1.f }; // these must be whole numbers, and all the same, and chosen so that size / count > sense_radius

int main(int argc, char** argv) {
    auto s = time(nullptr);
    srand(s);
    std::cout << "Random seed is " << s << std::endl;

    InitWindow(width, height, "Boids");

    auto cell_size = world_size / cell_count;
    size_t total_cell_count = static_cast<size_t>(cell_count.x) * static_cast<size_t>(cell_count.y) * static_cast<size_t>(cell_count.z);

    // load vertex/fragment shader
    Shader draw_shader = LoadShader("shaders/boids.vert", "shaders/boids.frag");

    // load boid update shader
    auto *cs_src = LoadFileText("shaders/boids.comp");
    auto cs_compiled = rlCompileShader(cs_src, RL_COMPUTE_SHADER);
    auto boid_update_shader = rlLoadComputeShaderProgram(cs_compiled);
    UnloadFileText(cs_src);

    // load first grid pass shader
    cs_src = LoadFileText("shaders/grid_pass1.comp");
    cs_compiled = rlCompileShader(cs_src, RL_COMPUTE_SHADER);
    auto grid_pass1 = rlLoadComputeShaderProgram(cs_compiled);
    UnloadFileText(cs_src);

    auto *positions = static_cast<Vector4*>(RL_CALLOC(num_boids, sizeof(Vector4)));
    auto *rotations = static_cast<Vector4*>(RL_CALLOC(num_boids, sizeof(Vector4)));
    auto *segment_start = static_cast<int*>(RL_CALLOC(total_cell_count + 1, sizeof(int))); // where does the segment for a given cell start
    auto *segment_length = static_cast<int*>(RL_CALLOC(total_cell_count + 1, sizeof(int))); // how long is the segment for a given cell
    auto *ids_sorted = static_cast<int*>(RL_CALLOC(num_boids, sizeof(int))); // boid ids sorted by the grid cell id they're in
    for (int i = 0; i < num_boids; i++) {
        positions[i].x = randfloat() - 0.5f;
        positions[i].y = randfloat() - 0.5f;
        positions[i].z = randfloat() - 0.5f;

        rotations[i].x = randfloat() - 0.5f;
        rotations[i].y = randfloat() - 0.5f;
        rotations[i].z = randfloat() - 0.5f;
    }
    auto ssbo0 = rlLoadShaderBuffer(num_boids * sizeof(Vector4), positions, RL_DYNAMIC_COPY);
    auto ssbo1 = rlLoadShaderBuffer(num_boids * sizeof(Vector4), rotations, RL_DYNAMIC_COPY);
    auto ssbo2 = rlLoadShaderBuffer((total_cell_count + 1) * sizeof(int), segment_start, RL_DYNAMIC_COPY);
    auto ssbo3 = rlLoadShaderBuffer((total_cell_count + 1) * sizeof(int), segment_length, RL_DYNAMIC_COPY);
    auto ssbo4 = rlLoadShaderBuffer(num_boids * sizeof(int), ids_sorted, RL_DYNAMIC_COPY);

    auto agent_vao = init_agent_mesh();

    Camera camera = {
        { 2 * world_size.x, 2 * world_size.y, 2 * world_size.z},
        { 0, 0, 0 },
        {0, 1, 0},
        35.0f,
        CAMERA_PERSPECTIVE
    };
    float boidScale = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime() * time_scale;
        // events
        PollInputEvents();
        move_camera(camera, cam_speed, dt, time_scale);
        if (IsKeyDown(KEY_SPACE)) draw_bounds = !draw_bounds;

        // spatial partitioning
        // have to clear the buffers first!
        // is there a better way to do this?
        memset(segment_start, 0, (total_cell_count + 1) * sizeof(int));
        memset(segment_length, 0, (total_cell_count + 1) * sizeof(int));
        memset(ids_sorted, 0, num_boids * sizeof(int));
        // figure out the lookup table
        // count the number of occupants of each cell
        // easy parallelization
        for (int i = 0; i < num_boids; i++) {
            auto pos_grid = xyz(positions[i]) + 0.5f * world_size;
            auto cell_3d = cell_index_3d(pos_grid, cell_size);
            auto cell = cell_index_1d(cell_3d, cell_count);
            ++segment_start[cell];
            ++segment_length[cell];
        }
        // prefix sum
        for (int i = 1; i <= total_cell_count; i++) {
            segment_start[i] += segment_start[i - 1];
        }
        // sort into the output array
        for (int i = num_boids - 1; i >= 0; i--) {
            auto pos_grid = xyz(positions[i]) + 0.5f * world_size;
            auto cell = cell_index_1d(cell_index_3d(pos_grid, cell_size), cell_count);
            ids_sorted[--segment_start[cell]] = i;
        }
        // send results to the gpu
        rlUpdateShaderBuffer(ssbo2, segment_start, (total_cell_count + 1) * sizeof(int), 0);
        rlUpdateShaderBuffer(ssbo3, segment_length, (total_cell_count + 1) * sizeof(int), 0);
        rlUpdateShaderBuffer(ssbo4, ids_sorted, num_boids * sizeof(int), 0);

        // boid update pass
        rlEnableShader(boid_update_shader);
        // set shader params
        rlSetUniform(0, &dt, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(1, &boid_speed, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(2, &boid_sense_radius, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(3, &rotation_speed, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(4, &cohesion_wt, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(5, &avoidance_wt, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(6, &alignment_wt, SHADER_UNIFORM_FLOAT, 1);
        rlSetUniform(7, &world_size, RL_SHADER_UNIFORM_VEC3, 1);
        rlSetUniform(8, &cell_count, RL_SHADER_UNIFORM_VEC3, 1);
        // bind buffers to compute shader
        // buffers seem to be being passed right
        // what else could be breaking??
        rlBindShaderBuffer(ssbo0, 0);
        rlBindShaderBuffer(ssbo1, 1);
        rlBindShaderBuffer(ssbo2, 2);
        rlBindShaderBuffer(ssbo3, 3);
        rlBindShaderBuffer(ssbo4, 4);
        // dispatch workgroups
        rlComputeShaderDispatch(num_boids / 1024 + num_boids % 1024, 1, 1);
        // done
        rlDisableShader();
        rlCheckErrors();

        // draw pass
        BeginDrawing();
        ClearBackground(BLACK);
        BeginMode3D(camera);
        rlEnableShader(draw_shader.id);

        Matrix projection = rlGetMatrixProjection();
        Matrix view = GetCameraMatrix(camera);

        // send parameters to draw shader
        SetShaderValueMatrix(draw_shader, 0, projection);
        SetShaderValueMatrix(draw_shader, 1, view);
        SetShaderValue(draw_shader, 2, &boidScale, SHADER_UNIFORM_FLOAT);

        // send buffers to draw shader
        rlBindShaderBuffer(ssbo0, 0);
        rlBindShaderBuffer(ssbo1, 1);

        // draw the boids, instanced
        rlEnableVertexArray(agent_vao);
        rlDrawVertexArrayInstanced(0, 3, num_boids);
        rlDisableVertexArray();
        rlDisableShader();

        if (draw_bounds) {
            DrawCubeWiresV((Vector3){0, 0, 0}, world_size, DARKGRAY);
        }
        EndMode3D();

        // gui pass
        if (draw_bounds) {
            GuiSlider((Rectangle){(float)width - 250, 10, 200, 10}, "Sense radius", TextFormat("%.5f", boid_sense_radius), &boid_sense_radius, 0, 1.5f);
            GuiSlider((Rectangle){(float)width - 250, 25, 200, 10}, "Move speed", TextFormat("%.5f", boid_speed), &boid_speed, 0, 1.5f);
            GuiSlider((Rectangle){static_cast<float>(width) - 250, 40, 200, 10}, "Turn speed", TextFormat("%.5f", rotation_speed), &rotation_speed, 0, 1.5f);
            GuiSlider((Rectangle){static_cast<float>(width) - 250, 55, 200, 10}, "Cohesion weight", TextFormat("%.5f", cohesion_wt), &cohesion_wt, 0, 2);
            GuiSlider((Rectangle){static_cast<float>(width) - 250, 70, 200, 10}, "Avoidance weight", TextFormat("%.5f", avoidance_wt), &avoidance_wt, 0, 2);
            GuiSlider((Rectangle){static_cast<float>(width) - 250, 85, 200, 10}, "Alignment weight", TextFormat("%.5f", alignment_wt), &alignment_wt, 0, 2);
            GuiSlider((Rectangle){static_cast<float>(width) - 250, 100, 200, 10}, "Time scale", TextFormat("%.2f", time_scale), &time_scale, 0, 2);
        }

        DrawFPS(0, 0);
        DrawText(TextFormat("%zu agents", num_boids), 0, 20, 18, DARKGREEN);

        EndDrawing();
    }

    RL_FREE(positions);
    RL_FREE(rotations);
    RL_FREE(segment_start);
    RL_FREE(segment_length);
    RL_FREE(ids_sorted);
    CloseWindow();

    return 0;
}
