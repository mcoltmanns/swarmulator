//
// Created by moltmanns on 4/13/25.
//

#include "Sphere.h"

#include "raymath.h"


namespace swarmulator::env {
    bool Sphere::intersect(const Vector3 &point, const float rad) const {
        return std::abs(Vector3Distance(position_, point) - rad) < radius_;
    }
} // env
// swarmulator