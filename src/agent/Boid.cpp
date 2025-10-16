//
// Created by moltmanns on 4/23/25.
//

#include "Boid.h"

#include <iostream>

#include "raymath.h"
#include "../sim/util.h"


namespace swarmulator {
     void Boid::update(const std::vector<SimObject*> &neighborhood, const float dt) {
        Vector3 cohesion = {0, 0, 0};
        u_int32_t coc = 0;
        Vector3 avoidance = {0, 0, 0};
        u_int32_t alc = 0;
        Vector3 alignment = {0, 0, 0};

        for (auto neighbor : neighborhood) {
            // do stuff
            // default behavior is boids
            if (neighbor == this || dynamic_cast<Boid*>(neighbor) == nullptr) { // skip if this is you (shouldn't be necessary), or if this is not a boid
                continue;
            }
            const auto diff = position_ - neighbor->get_position();
            const auto dist = Vector3Distance(neighbor->get_position(), position_);
            if (dist < interaction_radius_ / 2.f) {
                auto d = Vector3Length(diff);
                avoidance = avoidance + diff / (1 + d); // watch the divide by 0!
            }
            if (dist < interaction_radius_) {
                cohesion = cohesion + neighbor->get_position();
                coc++;
                alignment = alignment + neighbor->get_rotation();
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

        const auto steer_dir = cohesion_wt_ * cohesion + avoidance_wt_ * avoidance + alignment_wt_ * (alignment - rotation_);

        const float ip = std::exp(-7. * dt);

        rotation_ = Vector3Lerp(steer_dir, Vector3Normalize(rotation_), ip);
        position_ = position_ + rotation_ * 15 * dt;
        if (std::isnan(position_.x) || std::isnan(position_.y) || std::isnan(position_.z)) {
            throw std::runtime_error("Boid update position gave NaN");
        }
    }
}
// swarmulator