//
// Created by moltmanns on 4/23/25.
//

#ifndef BOID_H
#define BOID_H
#include "../sim/SimObject.h"

namespace swarmulator {

class Boid final : public SimObject {
private:
    float cohesion_wt_ = 0.75;
    float avoidance_wt_ = 1;
    float alignment_wt_ = 0.5;

public:
    Boid() = default;
    Boid(const Vector3 position, const Vector3 rotation) : SimObject(position, rotation) {}

    void update(const std::list<SimObject *> &neighborhood, float dt) override;

    std::string type_name() const override { return "Boid"; };
    std::vector<float> log() const override { return  { static_cast<float>(id_), position_.x, position_.y, position_.z }; }
};

}
// swarmulator

#endif //BOID_H
