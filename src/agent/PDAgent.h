//
// Created by moltmanns on 4/28/25.
//

#ifndef PDAGENT_H
#define PDAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class PDAgent final : public NeuralAgent {
private:
    int team = std::round(randfloat()); // 0 for cooperate, 1 for defect
    float coop_payoff = 0.2f;
    float defect_payoff = 0.4f;

public:
    explicit PDAgent() : NeuralAgent() {};
    PDAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {};
    ~PDAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &object, float dt) override;
    void to_ssbo(SSBOAgent *out) const override; // coop/defect decision is info.x

    [[nodiscard]] int get_team() const { return team; }
};

} // agent
// swarmulator

#endif //PDAGENT_H
