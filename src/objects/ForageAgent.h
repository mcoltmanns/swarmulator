//
// Created by moltma on 10/27/25.
//

#ifndef SWARMULATOR_CPP_FORAGEAGENT_H
#define SWARMULATOR_CPP_FORAGEAGENT_H
#include "NeuralAgent.h"

namespace swarmulator {

    class ForageAgent : public NeuralAgent {
    public:
        ForageAgent();
        ForageAgent(Vector3 position, Vector3 rotation);
        ~ForageAgent() override = default;

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override;

        [[nodiscard]] std::string type_name() const override { return "ForageAgent"; }
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_FORAGEAGENT_H
