//
// Created by moltmanns on 4/13/25.
//

#ifndef SPHERE_H
#define SPHERE_H
#include "raylib.h"


namespace swarmulator::env {

class Sphere {
private:
    Vector3 position_ = Vector3(0, 0, 0);
    float radius_ = 0.f;
    Color color_ = Color(0, 0, 0);

public:
    Sphere() = default;
    Sphere(const Vector3& position, const float radius, const Color color) : position_(position), radius_(radius), color_(color) {}
    ~Sphere() = default;

    [[nodiscard]] Vector3 position() const { return position_; }
    [[nodiscard]] float radius() const { return radius_; }

    void setPosition(const Vector3& position) { position_ = position; }
    void setRadius(const float radius) { radius_ = radius; }

    bool intersect(const Vector3& point) const;

    // the idea is that there won't be super many spheres, so we don't lose on much by using raylib draw calls
    void draw() const { DrawSphere(position_, radius_, color_); };
};

} // env
// swarmulator

#endif //SPHERE_H
