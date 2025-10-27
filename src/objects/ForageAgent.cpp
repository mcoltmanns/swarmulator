//
// Created by moltma on 10/27/25.
//

#include "ForageAgent.h"

#include "../sim/Simulation.h"

namespace swarmulator {
    ForageAgent::ForageAgent() : NeuralAgent() {
        interaction_radius_ = 25;
    }

     ForageAgent::ForageAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
        interaction_radius_ = 25;
    }

    void ForageAgent::update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) {
        NeuralAgent::update(context, neighborhood, dt); // do normal update (think and move, update energy from base and signal cost)

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
        // if you died or exceeded max lifetime, deactivate yourself (will be removed at next update)
        if (energy_ <= 0 || context.get_sim_time() - time_born_ > max_lifetime_) {
            deactivate();
        }
    }

} // namespace swarmulator
