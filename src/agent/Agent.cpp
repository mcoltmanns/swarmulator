//
// Created by moltmanns on 4/2/25.
//

#include "Agent.h"

#include <iostream>

#include "../env/Sphere.h"
#include "raymath.h"
#include "v3ops.h"

namespace swarmulator::agent {
    std::shared_ptr<Agent> Agent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                       const std::vector<std::shared_ptr<env::Sphere>> &objects,
                       const float dt) {
        position_ = position_ + direction_ * move_speed_ * dt;
        return nullptr;
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