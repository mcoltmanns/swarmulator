//
// Created by moltmanns on 4/23/25.
//

#include "Boid.h"

#include <iostream>

#include "v3ops.h"

#include "raymath.h"


namespace swarmulator::agent {
    std::shared_ptr<SimObject> Boid::update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
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
            const auto diff = position_ - neighbor->get_position();
            const auto dist = Vector3Distance(neighbor->get_position(), position_);
            if (dist < sense_radius_ / 2.f) {
                auto d = Vector3Length(diff);
                avoidance = avoidance + diff / (1 + d); // watch the divide by 0!
            }
            if (dist < sense_radius_) {
                cohesion = cohesion + neighbor->get_position();
                coc++;
                alignment = alignment + neighbor->get_direction();
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
        if (std::isnan(position_.x) || std::isnan(position_.y) || std::isnan(position_.z)) {
            std::cout << "uh oh!" << std::endl;
        }
        return nullptr;
    }
} // agent
// swarmulator