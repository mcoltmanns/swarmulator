//
// Created by moltma on 10/27/25.
//

#include "PDAgent.h"
#include "raymath.h"

namespace swarmulator {
    PDAgent::PDAgent() : NeuralAgent() {
        interaction_radius_ = 2.5;
    }

    PDAgent::PDAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
        interaction_radius_ = 2.5;
    }

    void PDAgent::update(swarmulator::Simulation &context, const std::list<SimObject *> &neighborhood, float dt) {
        NeuralAgent::update(context, neighborhood, dt); // do normal update (think, move update energy based on costs)

        // set your team according to what the network decided
        team_ = static_cast<int>(std::round(output_(0, 4)));

        // update decision payoffs according to neighbors
        float coop_pay = 0;
        float defect_pay = 0;
        for (const auto& thing : neighborhood) {
            if (const auto neighbor = dynamic_cast<PDAgent*>(thing); neighbor != nullptr) {
                const float factor = 1.f / (1.f + Vector3Distance(position_, neighbor->get_position()));
                if (neighbor->get_team() == 0) {
                    coop_pay += coop_payoff * factor;
                    defect_pay += defect_payoff * factor; // TODO dynamic payoff tuning (to keep population alive)
                }
                else {
                    coop_pay -= coop_payoff * factor;
                }
            }
        }

        // update energy based on choice
        if (team_ == 0) {
            energy_ += coop_pay * dt;
        }
        else {
            energy_ += defect_pay * dt;
        }
    }

    std::vector<float> PDAgent::log() const { return NeuralAgent::log(); }
}