//
// Created by moltma on 11/21/25
//

#include "Prey.h"

#include "Plant.h"
#include "../sim/Simulation.h"

namespace swarmulator {
    Prey::Prey() : NeuralAgent() {
        interaction_radius_ = 25;
        max_lifetime_ = 10000;
    }

    Prey::Prey(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
        interaction_radius_ = 25;
        max_lifetime_ = 10000;
    }
    
    void Prey::update(Simulation &context, const std::list<SimObject *> &neighborhood, const float dt) {
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
        // take energy from nearby plants, inverse weighted depending on distance
        for (const auto object : neighborhood) {
            if (const auto plant = dynamic_cast<Plant *>(object); plant != nullptr) {
                const float base = 1.f / (1.f + Vector3Distance(position_, plant->get_position()));
                change_energy(base * dt * eat_rate_);
                plant->change_energy(-base * dt * eat_rate_);
            }
        }
    }
}

