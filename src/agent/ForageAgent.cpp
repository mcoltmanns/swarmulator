//
// Created by moltmanns on 4/22/25.
//

#include "ForageAgent.h"


namespace swarmulator::agent {
    std::shared_ptr<Agent> ForageAgent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
        const std::list<std::shared_ptr<env::Sphere>> &objects, const float dt) {
        NeuralAgent::update(neighborhood, objects, dt); // perform your normal update (think and move)

        // if you're touching a sphere, eat
        for (const auto& sphere : objects) {
            if (sphere->intersect(position_, scale_)) {
                //energy_ += eat_energy_ * dt;
                energy_ = 1;
                sphere->setColor(YELLOW);
            }
        }

        // if you have the energy, reproduce
        if (energy_ > reproduction_threshold_) {
            return reproduce();
        }
        return nullptr;
    }
} // agent
// swarmulator