//
// Created by moltma on 11/21/25
//

#include "Prey.h"

#include "Plant.h"
#include "../sim/Simulation.h"

namespace swarmulator {
    Prey::Prey() : NeuralAgent() {
        interaction_radius_ = 25;
    }

    Prey::Prey(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
        interaction_radius_ = 25;
    }
    
    void Prey::update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) {
        NeuralAgent::update(context, neighborhood, dt); // do normal update (think and move, update energy from base and signal const)

        // if you can reproduce, do it
        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto child = *this;
            child.position_ = {
                randfloat(-context.get_world_size().x / 2.f, context.get_world_size().x / 2.f),
                randfloat(-context.get_world_size().y / 2.f, context.get_world_size().y / 2.f),
                randfloat(-context.get_world_size().z / 2.f, context.get_world_size().z / 2.f)
            };
            child.rotation_ = { randfloat(-1, 1), randfloat(-1, 1), randfloat(-1, 1) };
            child.rotation_ = Vector3Normalize(child.rotation_);
            child.mutate();
            child.parent_id_ = id_;
            child.time_born_ = context.get_sim_time();
            child.energy_ = initial_energy_;
            context.add_object(child);
        }

        // if you're near a plant, eat from it
        // prey always eat when they can
        for (const auto neighbor : neighborhood) {
            if (const auto plant = dynamic_cast<Plant*>(neighbor); plant != nullptr) {
                // if the plant isn't dead
                if (plant->get_energy() > 0) {
                    // take the energy from it
                    plant->change_energy(eat_rate_ * dt);
                    // and give it to yourself
                    energy_ += eat_rate_ * dt;
                }
            }
        }
    }
}

