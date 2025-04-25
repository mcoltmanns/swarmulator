//
// Created by moltmanns on 4/22/25.
//

#ifndef FORAGEAGENT_H
#define FORAGEAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class ForageAgent final : public NeuralAgent {
private:
    float eat_energy_ = 5; // how much energy do you get from eating (per second spent in the sphere)

public:
    ForageAgent() = default;
    ForageAgent(const Vector3 position, const Vector3 rotation, const float scale) : NeuralAgent(position, rotation, scale) {}
    ~ForageAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;
};

} // agent
// swarmulator

#endif //FORAGEAGENT_H
