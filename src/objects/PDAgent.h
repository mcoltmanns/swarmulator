//
// Created by moltma on 10/27/25.
//

#ifndef SWARMULATOR_CPP_PDAGENT_H
#define SWARMULATOR_CPP_PDAGENT_H
#include "NeuralAgent.h"


namespace swarmulator {
    class PDAgent final : public swarmulator::NeuralAgent {
    private:
        int team_ = 0; // 0 is cooperate, 1 is defect

        static constexpr float coop_payoff = 0.8f; // defect payoff should always be higher than coop payoff
        static constexpr float defect_payoff = 1.f;

    public:
        PDAgent();
        PDAgent(Vector3 position, Vector3 rotation);
        ~PDAgent() override = default;

        [[nodiscard]] auto get_team() const { return team_; }

        void update(swarmulator::Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override;

        [[nodiscard]] std::vector<float> log() const override;

        [[nodiscard]] SimObject::SSBOObject to_ssbo() const override;
    };
}

#endif // SWARMULATOR_CPP_PDAGENT_H
