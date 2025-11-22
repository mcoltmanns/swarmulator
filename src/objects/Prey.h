//
// Created by moltma on 11/21/25
//

#ifndef SWARMULATOR_CPP_PREY_H
#define SWARMULATOR_CPP_PREY_H

#include "NeuralAgent.h"

namespace swarmulator {
    class Prey : public NeuralAgent {
    protected:
        const float eat_rate_ = 1; // how much energy the prey can sap from a plant per unit time
    public:
        Prey();
        Prey(Vector3 position, Vector3 rotation);
        ~Prey() override = default;

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override;

        [[nodiscard]] std::string type_name() const override { return "Prey"; }
    };
}

#endif

