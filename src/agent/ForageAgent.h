//
// Created by moltmanns on 4/22/25.
//

#ifndef FORAGEAGENT_H
#define FORAGEAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class DiscreteForageAgent final : public NeuralAgent {
private:
    float eat_energy_ = 10; // how much energy do you get from eating (per second spent in the sphere)

public:
    DiscreteForageAgent() : NeuralAgent() {};
    DiscreteForageAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {}
    ~DiscreteForageAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;
};

class ContinuousForageAgent final : public NeuralAgent {
private:
    float eat_energy_ = 1; // how much energy do you get from eating per second/per update (weighted by inverse distance to food source center)

public:
    ContinuousForageAgent() : NeuralAgent() {};
    ContinuousForageAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {}
    ~ContinuousForageAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;
};

} // agent
// swarmulator

#endif //FORAGEAGENT_H
