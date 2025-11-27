//
// Created by moltma on 11/21/25
//

#ifndef SWARMULATOR_CPP_PREDATOR_H
#define SWARMULATOR_CPP_PREDATOR_H

#include "NeuralAgent.h"

namespace swarmulator {
    class Predator : public NeuralAgent {
    protected:
        const float chase_cost_ = 0.01; // how much extra energy it costs per unit time to chase prey
    public:
        Predator();
        Predator(Vector3 position, Vector3 rotation);
        ~Predator() override = default;

        void update(Simulation &context, const std::list<SimObject*> &neighborhood, float dt) override;

        [[nodiscard]] std::string type_name() const override { return "Predator"; }
    };
}

#endif
