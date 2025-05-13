//
// Created by moltmanns on 4/22/25.
//

#include "ForageAgent.h"

#include <iostream>

#include "raymath.h"


namespace swarmulator::agent {
    std::shared_ptr<Agent> DiscreteForageAgent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
        const std::list<std::shared_ptr<env::Sphere>> &objects, const float dt) {
        NeuralAgent::update(neighborhood, objects, dt); // perform your normal update (think and move)

        // if you're touching a sphere, eat
        for (const auto& sphere : objects) {
            if (sphere->intersect(position_, scale)) {
                energy_ += global_reward_factor * eat_energy_ * dt / Vector3Distance(position_, sphere->position()); // scale the energy you get by the inverse distance to the sphere center
                sphere->setColor(YELLOW);
            }
        }

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

    std::shared_ptr<Agent> ContinuousForageAgent::update(const std::vector<std::shared_ptr<Agent> > &neighborhood, const std::list<std::shared_ptr<env::Sphere> > &objects, const float dt) {
        NeuralAgent::update(neighborhood, objects, dt); // perform normal update

        float extra = 0;
        const float reward_per_sphere = global_reward_factor * eat_energy_;// / objects.size();
        for (const auto& sphere : objects) {
            extra += reward_per_sphere / Vector3Distance(position_, sphere->position());
        }
        energy_ += extra * dt;

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