//
// Created by moltmanns on 4/23/25.
//

#ifndef BOID_H
#define BOID_H
#include "Agent.h"

namespace swarmulator::agent {

class Boid final : public Agent {
private:
    float cohesion_wt_ = 1;
    float avoidance_wt_ = 1;
    float alignment_wt_ = 1;

public:
    Boid() = default;
    Boid(const Vector3& position, const Vector3& direction) : Agent(position, direction) {}
    ~Boid() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                                  const std::vector<std::shared_ptr<env::Sphere>> &objects, float dt) override;
};

} // agent
// swarmulator

#endif //BOID_H
