//
// Created by moltmanns on 3/28/25.
//

#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <rcamera.h>
#include <string>

// get a random float between 0 and 1
inline float randfloat() {
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

/*inline unsigned int init_agent_mesh() {
    const auto vao = rlLoadVertexArray();
    rlEnableVertexArray(vao);
    // base mesh is a triangle in the unit circle, which we modify in the vertex shader
    Vector3 mesh[] = {
        { -0.86f, -0.5f, 0.0f },
        { 0.86f, -0.5f, 0.0f },
        { 0.0f, 1.0f, 0.0f }
    };
    // load the vao with one instance of the mesh
    rlEnableVertexAttribute(0);
    rlLoadVertexBuffer(mesh, sizeof(mesh), false); // the mesh is not dynamic
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlDisableVertexArray();
    return vao;
}*/

inline void move_camera(Camera3D &camera, const float speed, const float dt, const float time_scale) {
    auto cam_speed_factor = time_scale == 0 ? dt : (dt / time_scale);
    if (IsKeyDown(KEY_D)) CameraYaw(&camera, speed * cam_speed_factor, true);
    if (IsKeyDown(KEY_A)) CameraYaw(&camera, -speed * cam_speed_factor, true);
    if (IsKeyDown(KEY_W)) CameraPitch(&camera, -speed * cam_speed_factor, true, true, false);
    if (IsKeyDown(KEY_S)) CameraPitch(&camera, speed * cam_speed_factor, true, true, false);
    if (IsKeyDown(KEY_Q)) CameraMoveToTarget(&camera, speed * cam_speed_factor);
    if (IsKeyDown(KEY_E)) CameraMoveToTarget(&camera, -speed * cam_speed_factor);
}

inline char* get_opt(char** begin, char** end, const std::string& option) {
    char** it = std::find(begin, end, option);
    if (it != end && ++it != end) {
        return *it;
    }
    return nullptr;
}

inline bool opt_exists(char** begin, char** end, const std::string& option) {
    return std::find(begin, end, option) != end;
}

/*inline int cell_index_1d(const Vector3 &cell, const Vector3 &num_cells) {
    auto xpart = static_cast<int>(num_cells.x) * static_cast<int>(num_cells.x) * static_cast<int>(cell.x);
    auto ypart = static_cast<int>(num_cells.y) * static_cast<int>(cell.y);
    auto zpart = static_cast<int>(cell.z);
    return xpart + ypart + zpart;
}
inline Vector3 cell_index_3d(const Vector3 &p, const Vector3 &cell_delta) { return floorv3(p / cell_delta); }*/

#endif //UTIL_H
