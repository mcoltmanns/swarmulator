//
// Created by moltmanns on 5/5/25.
//

#include "PredatorAgent.h"

#include "PreyAgent.h"
#include "raymath.h"

namespace swarmulator::agent {
    std::shared_ptr<Agent> PredatorAgent::update(
        const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects,
        const float dt) {
        NeuralAgent::update(neighborhood, objects, dt); // perform normal update

        // eat the first prey agent in your sense radius you find
        for (const auto& n_a : neighborhood) {
            auto prey = std::dynamic_pointer_cast<PreyAgent>(n_a);
            if (prey == nullptr) {
                continue;
            }
            if (const auto dist = Vector3DistanceSqr(position_, prey->get_position()); dist <= sense_radius_ * sense_radius_) {
                set_energy(get_energy() + prey->get_energy()); // add the prey's energy to yours
                prey->set_energy(-999); // kill the prey
                break;
            }
        }

        // if you can, reproduce
        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto n_a = std::make_shared<PredatorAgent>(*this);
            n_a->set_energy(initial_energy_);
            n_a->set_time_born(GetTime());
            n_a->mutate();
            return n_a;
        }
        return nullptr;
    }

    void PredatorAgent::to_ssbo(SSBOAgent *out) const {
        NeuralAgent::to_ssbo(out);
        out->info.x = 1;
    }
}
