//
// Created by moltmanns on 5/5/25.
//

#ifndef PREYAGENT_H
#define PREYAGENT_H
#include "NeuralAgent.h"


// prey agents gain energy from clumping together and have a low base survival cost, but can be eaten by predators
namespace swarmulator::agent {
    class PreyAgent final : public NeuralAgent {
    private:
        float proximity_payoff_ = 1.f;
        float basic_cost_ = 0.01f;

    public:
        explicit PreyAgent() : NeuralAgent() {};
        PreyAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {};
        ~PreyAgent() override = default;

        std::shared_ptr<SimObject>
        update(const std::vector<std::shared_ptr<SimObject> > &neighborhood, float dt) override;
        void to_ssbo(SSBOSimObject *out) const override; // predator/prey is info.x (0 for prey, 1 for pred)

        [[nodiscard]] static bool is_edible() { return true; }
    };
}

#endif //PREYAGENT_H
