//
// Created by moltmanns on 5/5/25.
//

#include "PreyAgent.h"

#include "raymath.h"

namespace swarmulator::agent {
    std::shared_ptr<SimObject> PreyAgent::update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                                 float dt) {
        NeuralAgent::update(neighborhood, dt); // perform normal update

        float payoff = 0;
        for (const auto &n_a : neighborhood) {
            const auto neighbor = std::dynamic_pointer_cast<PreyAgent>(n_a);
            if (neighbor == nullptr || neighbor.get() == this) {
                continue;
            }
            if (const auto dist = Vector3DistanceSqr(position_, neighbor->get_position()); dist <= sense_radius_ * sense_radius_) {
                const float factor = global_reward_factor / (1.f + std::sqrt(dist));
                payoff += proximity_payoff_ * factor;
            }
        }

        energy_ += payoff * dt;
        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<PreyAgent>(*this);
            n_a->set_energy(initial_energy_);
            n_a->mutate();
            n_a->parent_ = id_;
            return n_a;
        }

        return nullptr;
    }

    void PreyAgent::to_ssbo(SSBOSimObject *out) const {
        NeuralAgent::to_ssbo(out);
        out->info_b.x = 0;
    }
}
