//
// Created by moltmanns on 4/2/25.
//

#include "SimObject.h"

#include <iostream>

#include "../env/Sphere.h"
#include "raymath.h"
#include "v3ops.h"

namespace swarmulator::agent {
    std::shared_ptr<SimObject> SimObject::update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                                 const float dt) {
        position_ = position_ + direction_ * move_speed_ * dt;
        return nullptr;
    }

    void SimObject::to_ssbo(SSBOSimObject *out) const {
        out->position.x = position_.x;
        out->position.y = position_.y;
        out->position.z = position_.z;
        out->direction.x = direction_.x;
        out->direction.y = direction_.y;
        out->direction.z = direction_.z;
    }
} // swarmulator