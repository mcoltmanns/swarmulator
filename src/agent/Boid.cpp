//
// Created by moltmanns on 4/23/25.
//

#include "Boid.h"

#include "raymath.h"
#include "../sim/util.h"
#include "../sim/Simulation.h"

namespace swarmulator {
    void Boid::update(Simulation &context, const std::list<SimObject *> &neighborhood, const float dt) {
        Vector3 cohesion = {0, 0, 0};
        u_int32_t coc = 0;
        Vector3 avoidance = {0, 0, 0};
        u_int32_t alc = 0;
        Vector3 alignment = {0, 0, 0};

        for (const auto neighbor : neighborhood) {
            if (dynamic_cast<Boid*>(neighbor) != nullptr) {
                // if this is a boid do boid stuff
                const auto diff = position_ - neighbor->get_position();
                const auto dist = Vector3Distance(neighbor->get_position(), position_);
                if (dist < interaction_radius_ / 2.f) { // if the other agent is really close to us, avoid it
                    auto d = Vector3Length(diff);
                    avoidance = avoidance + diff / (1 + d); // watch the divide by 0!
                }
                // always do cohesion and alignment
                cohesion = cohesion + neighbor->get_position();
                coc++;
                alignment = alignment + neighbor->get_rotation();
                alc++;
            }
            else if (dynamic_cast<BoidEffector*>(neighbor) != nullptr) {
                // if this is a boid effector just do avoidance
                const auto diff = position_ - neighbor->get_position();
                auto d = Vector3Length(diff);
                avoidance = avoidance + (10 * (diff / (1 + d))); // be REALLY scared of effectors
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

        const float ip = std::expf(-7 * dt);

        rotation_ = Vector3Lerp(steer_dir, Vector3Normalize(rotation_), ip);
        position_ = position_ + rotation_ * 15 * dt;
        if (std::isnan(position_.x) || std::isnan(position_.y) || std::isnan(position_.z)) {
            throw std::runtime_error("Boid update position gave NaN");
        }

        if (randfloat() < 0.000001f) {
            context.add_object(Boid(*this));
        }
    }
}
// swarmulator