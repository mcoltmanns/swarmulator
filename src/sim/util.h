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
#include <sstream>

namespace swarmulator {
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

    inline std::string Vector4ToString(const Vector4 &v) {
        std::stringstream ss;
        ss << "(" << v.x << " " << v.y << " " << v.z << " " << v.w << ")";
        return ss.str();
    }

    inline std::string Vector3ToString(const Vector3 &v) {
        std::stringstream ss;
        ss << "(" << v.x << ", " << v.y << ", " << v.z << ")";
        return ss.str();
    }

    inline std::string Vector2ToString(const Vector2 &v) {
        std::stringstream ss;
        ss << "(" << v.x << ", " << v.y << ")";
        return ss.str();
    }

    // couple vector3 operators that raymath doesn't provide
    inline Vector3 operator*(const float s, const Vector3 &r) { return Vector3(r.x * s, r.y * s, r.z * s); }
    inline Vector3 floorv3(const Vector3 &v) { return Vector3(floor(v.x), floorf(v.y), floorf(v.z)); }
    inline Vector3 xyz(const Vector4 &in) { return Vector3(in.x, in.y, in.z); }

    // wrap out of bounds positions to the opposite sides of the world
    [[nodiscard]] static Vector3 wrap_position(Vector3 position, const Vector3 &world_size) {
        if (position.x < -world_size.x / 2.f) {
            position.x = world_size.x / 2.f;
        }
        else if (position.x > world_size.x / 2.f) {
            position.x = -world_size.x / 2.f;
        }
        if (position.y < -world_size.y / 2.f) {
            position.y = world_size.y / 2.f;
        }
        else if (position.y > world_size.y / 2.f) {
            position.y = -world_size.y / 2.f;
        }
        if (position.z < -world_size.z / 2.f) {
            position.z = world_size.z / 2.f;
        }
        else if (position.z > world_size.z / 2.f) {
            position.z = -world_size.z / 2.f;
        }

        return position;
    }
}

#endif //UTIL_H
