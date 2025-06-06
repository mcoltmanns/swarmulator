//
// Created by moltmanns on 4/28/25.
//

#ifndef PDAGENT_H
#define PDAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class PDAgent final : public NeuralAgent {
private:
    int team = static_cast<int>(std::round(randfloat())); // 0 for cooperate, 1 for defect
    float coop_payoff = 10.f;
    float defect_payoff = 12.f; // d > c > 0
    float sense_radius_ = 2.5f; // has to be small for things to work good

public:
    explicit PDAgent() : NeuralAgent() {};
    PDAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {};
    ~PDAgent() override = default;

    std::shared_ptr<SimObject> update(const std::vector<std::shared_ptr<SimObject> > &neighborhood, float dt) override;
    void to_ssbo(SSBOSimObject *out) const override; // coop/defect decision is info.x

    [[nodiscard]] int get_team() const { return team; }
};

} // agent
// swarmulator

#endif //PDAGENT_H
