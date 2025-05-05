//
// Created by moltmanns on 5/5/25.
//

#ifndef PREDATORAGENT_H
#define PREDATORAGENT_H
#include "NeuralAgent.h"


// predator agents gain energy from eating prey agents, but have a higher energy cost
// when you eat a prey agent, you gain all its energy
namespace swarmulator::agent {
    class PredatorAgent final : public NeuralAgent{
    private:
        float basic_cost_ = 0.1f;

    public:
        explicit PredatorAgent() : NeuralAgent() {};
        PredatorAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {};
        ~PredatorAgent() override = default;

        std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;
        void to_ssbo(SSBOAgent *out) const override;

        [[nodiscard]] static bool is_edible() { return false; }
    };
}

#endif //PREDATORAGENT_H
