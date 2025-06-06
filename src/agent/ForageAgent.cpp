//
// Created by moltmanns on 4/22/25.
//

#include "ForageAgent.h"

#include <iostream>

#include "raymath.h"


namespace swarmulator::agent {
    std::shared_ptr<SimObject> DiscreteForageAgent::update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                                           const float dt) {
        NeuralAgent::update(neighborhood, dt); // perform your normal update (think and move)

        // if you have the energy, reproduce
        if (energy_ > reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<DiscreteForageAgent>(*this); // copy yourself
            n_a->set_energy(initial_energy_); // reset the copy's energy
            n_a->mutate(); // mutate the copy
            n_a->parent_ = id_;
            return n_a;
        }
        return nullptr;
    }

    std::shared_ptr<SimObject> ContinuousForageAgent::update(
        const std::vector<std::shared_ptr<SimObject> > &neighborhood, const float dt) {
        NeuralAgent::update(neighborhood, dt); // perform normal update

        float extra = 0;
        /*const float reward_per_sphere = global_reward_factor * eat_energy_ / static_cast<float>(objects.size());
        for (const auto& sphere : objects) {
            extra += reward_per_sphere / (1.f + Vector3Distance(position_, sphere->position()));
        }
        energy_ += extra * dt;*/

        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<ContinuousForageAgent>(*this);
            n_a->set_energy(initial_energy_);
            n_a->mutate();
            n_a->parent_ = id_;
            return n_a;
        }
        return nullptr;
    }


} // agent
// swarmulator