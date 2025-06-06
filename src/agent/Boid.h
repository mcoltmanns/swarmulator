//
// Created by moltmanns on 4/23/25.
//

#ifndef BOID_H
#define BOID_H
#include "SimObject.h"

namespace swarmulator::agent {

class Boid final : public SimObject {
private:
    float cohesion_wt_ = 1;
    float avoidance_wt_ = 1;
    float alignment_wt_ = 1;

public:
    Boid() = default;
    Boid(const Vector3& position, const Vector3& direction) : SimObject(position, direction) {}
    ~Boid() override = default;

    std::shared_ptr<SimObject> update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                      float dt) override;
};

} // agent
// swarmulator

#endif //BOID_H
