//
// Created by moltmanns on 4/28/25.
//

#include "PDAgent.h"

#include <iostream>

#include "raymath.h"

namespace swarmulator::agent {
    std::shared_ptr<SimObject> PDAgent::update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                               const float dt) {
        NeuralAgent::update(neighborhood, dt); // perform your normal update

        // set your team according to what the network decided
        team = static_cast<int>(std::round(output_(0, 4)));

        // update your energy according to your neighbors
        float coop_pay = 0;
        float defect_pay = 0;
        for (const auto& n_a: neighborhood) {
            const auto neighbor = dynamic_cast<PDAgent*>(n_a.get());
            if (neighbor == nullptr || neighbor == this) {
                continue;
            }
            if (const auto dist = Vector3DistanceSqr(position_, neighbor->position_); dist <= sense_radius_ * sense_radius_) {
                const float factor = global_reward_factor / (1.f + std::sqrt(dist));
                // if the neighbor is a cooperator, you get bonuses for cooperating and defecting
                if (neighbor->team == 0) {
                    coop_pay += coop_payoff * factor;
                    defect_pay += defect_payoff * factor;
                }
                // if they are a defector, the cooperation bonus drops, but the defector payoff does not
                else {
                    coop_pay -= coop_payoff * factor;
                }
            }
        }

        // update energy based on coop/defect choice and assoc. payoffs
        if (team == 0) {
            energy_ += coop_pay * dt;
        }
        else {
            energy_ += defect_pay * dt;
        }

        // if you have the energy, reproduce
        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<PDAgent>(*this);
            n_a->set_energy(initial_energy_);
            n_a->mutate();
            n_a->parent_ = id_;
            return n_a;
        }
        return nullptr;
    }

    void PDAgent::to_ssbo(SSBOSimObject *out) const {
        NeuralAgent::to_ssbo(out);
        out->info_a = Vector4{static_cast<float>(team), 0, 0, 0};
    }
} // swarmulator