//
// Created by moltmanns on 4/13/25.
//

#include "Sphere.h"

#include "raymath.h"


namespace swarmulator::env {
    bool Sphere::intersect(const Vector3 &point) const {
        return Vector3Distance(position_, point) < radius_;
    }
} // env
// swarmulator