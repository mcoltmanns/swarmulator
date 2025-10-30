//
// Created by moltma on 10/27/25.
//

#include "PDAgent.h"
#include "../sim/Simulation.h"
#include "raymath.h"

namespace swarmulator {
    PDAgent::PDAgent() : NeuralAgent() {
        interaction_radius_ = 5;
    }

    PDAgent::PDAgent(const Vector3 position, const Vector3 rotation) : NeuralAgent(position, rotation) {
        interaction_radius_ = 5;
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
                const float base = 1.f / (1.f + Vector3Distance(position_, neighbor->get_position()));
                const float tune = 1.f / (1.f + std::exp(0.5f * (static_cast<float>(context.get_total_num_objects()) - 500.f))); // tune is a logistic between 0-1 centered on the maximum number of agents we want
                // if the neighbor was cooperating, you get bonuses for cooperating and defecting
                if (neighbor->get_team() == 0) {
                    coop_pay += coop_payoff * base * tune;
                    defect_pay += defect_payoff * base * tune;
                }
                // if the neighbor defected, you only get a penalty for cooperating
                else {
                    coop_pay -= coop_payoff * base * tune;
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
    }

    std::vector<float> PDAgent::log() const { return NeuralAgent::log(); }

    SimObject::SSBOObject PDAgent::to_ssbo() const {
        auto base = swarmulator::NeuralAgent::to_ssbo();
        base.info.x = static_cast<float>(team_);
        return base;
    }
}
