//
// Created by moltma on 11/21/25
//

#ifndef SWARMULATOR_CPP_PREDATOR_H
#define SWARMULATOR_CPP_PREDATOR_H

#include "NeuralAgent.h"

namespace swarmulator {
    class Predator : public NeuralAgent {
    protected:
        const float chase_cost_ = 0.2; // how much extra energy it costs per unit time to chase prey
        int lifetime_pred_kills_ = 0;
        int lifetime_prey_kills_ = 0;
    public:
        Predator();
        Predator(Vector3 position, Vector3 rotation);
        ~Predator() override = default;

        void update(Simulation &context, const std::list<SimObject*> &neighborhood, float dt) override;

        [[nodiscard]] std::string type_name() const override { return "Predator"; }
        [[nodiscard]] std::vector<float> log() const override { auto base = NeuralAgent::log(); base.insert(base.end(), { static_cast<float>(lifetime_pred_kills_), static_cast<float>(lifetime_prey_kills_) }); return base; }
    };
}

#endif
