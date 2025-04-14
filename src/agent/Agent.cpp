//
// Created by moltmanns on 4/2/25.
//

#include "Agent.h"

#include <iostream>

#include "../env/Sphere.h"
#include "raymath.h"
#include "rlgl.h"
#include "v3ops.h"

namespace swarmulator::agent {
    Agent::Agent() = default;

    void Agent::update(const std::vector<Agent *> &neighborhood,
                       const std::vector<env::Sphere *> &objects,
                       const float dt) {
        Vector3 cohesion = {0, 0, 0};
        u_int32_t coc = 0;
        Vector3 avoidance = {0, 0, 0};
        u_int32_t alc = 0;
        Vector3 alignment = {0, 0, 0};

        for (const auto neighbor : neighborhood) {
            // do stuff
            // default behavior is boids
            if (neighbor == this) {
                continue;
            }
            const auto diff = position_ - neighbor->position_;
            const auto dist = Vector3Distance(neighbor->position_, position_);
            if (dist < sense_range_) {
                auto d = Vector3Length(diff);
                avoidance = avoidance + diff / d;
            }
            if (dist < sense_range_ * 2) {
                cohesion = cohesion + neighbor->position_;
                coc++;
                alignment = alignment + neighbor->direction_;
                alc++;
            }
        }

        if (coc > 0) {
            cohesion = cohesion / static_cast<float>(coc);
        }
        cohesion = Vector3Normalize(cohesion - position_);

        if (alc > 0) {
            alignment = alignment / static_cast<float>(alc);
        }

        const auto steer_dir = cohesion_wt_ * cohesion + avoidance_wt_ * avoidance + alignment_wt_ * (alignment - direction_);

        const float ip = std::exp(-rot_speed_ * dt);

        direction_ = Vector3Lerp(steer_dir, Vector3Normalize(direction_), ip);
        position_ = position_ + direction_ * 0.06 * dt; // 0.06 here is move speed
    }

    unsigned int Agent::vao_ = 0;

    void Agent::init_vao() {
        vao_ = rlLoadVertexArray();
        rlEnableVertexArray(vao_);
        constexpr Vector3 mesh[] = {
            {-0.86, -0.5, 0},
            {0.86, -0.5, 0},
            {0, 1, 0}
        };
        rlEnableVertexAttribute(vao_);
        rlLoadVertexBuffer(mesh, sizeof(mesh), false);
        rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
        rlDisableVertexArray();
    }
} // swarmulator