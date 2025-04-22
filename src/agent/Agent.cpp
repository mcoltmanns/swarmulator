//
// Created by moltmanns on 4/2/25.
//

#include "Agent.h"

#include <iostream>

#include "../env/Sphere.h"
#include "raymath.h"
#include "v3ops.h"

namespace swarmulator::agent {
    Agent::Agent() = default;

    void Agent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                       const std::vector<std::shared_ptr<env::Sphere>> &objects,
                       const float dt) {
        Vector3 cohesion = {0, 0, 0};
        u_int32_t coc = 0;
        Vector3 avoidance = {0, 0, 0};
        u_int32_t alc = 0;
        Vector3 alignment = {0, 0, 0};

        for (const auto &neighbor : neighborhood) {
            // do stuff
            // default behavior is boids
            if (neighbor.get() == this) {
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
        position_ = position_ + direction_ * move_speed_ * dt; // 0.06 here is move speed
    }

    void Agent::to_ssbo(SSBOAgent *out) const {
        out->position.x = position_.x;
        out->position.y = position_.y;
        out->position.z = position_.z;
        out->direction.x = direction_.x;
        out->direction.y = direction_.y;
        out->direction.z = direction_.z;
        out->signals.x = 0;
        out->signals.y = 0;
        out->info.x = 0;
        out->info.y = 0;
    }
} // swarmulator