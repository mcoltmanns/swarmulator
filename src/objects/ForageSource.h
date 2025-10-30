//
// Created by moltma on 10/23/25.
//

#ifndef SWARMULATOR_CPP_FORAGESOURCE_H
#define SWARMULATOR_CPP_FORAGESOURCE_H
#include "../sim/SimObject.h"
#include "../sim/Simulation.h"
#include "NeuralAgent.h"
#include "raymath.h"

namespace swarmulator {
    class ForageSource final : public SimObject {
    public:
        ForageSource() = default;
        ForageSource(const Vector3 p, const Vector3 r) : SimObject(p, r) {}
        ~ForageSource() override = default;

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, const float dt) override {
            interaction_radius_ = 50; // a little hacky and not very c++-like - what really should happen is we rewrite the constructors
            for (const auto& object : neighborhood) {
                // give energy to any nearby neural agents, inverse weighted depending on their distance to you
                if (const auto forager = dynamic_cast<NeuralAgent*>(object)) {
                    // base reward is inverse distance to source
                    const float base = 1.f / (1.f + Vector3Distance(position_, forager->get_position()));
                    // tune is a logistic between 0-1 centered on the maximum number of agents we want
                    const float tune = 1.f / (1.f + std::exp(0.5f * (static_cast<float>(context.get_total_num_objects()) - 500.f)));
                    forager->change_energy(base * tune * dt);
                }
            }

            // just add more agents if we run out
            if (context.get_total_num_objects() < 200) {
                auto new_agent = ForageAgent();
                new_agent.set_position({
                    randfloat(-context.get_world_size().x / 2.f, context.get_world_size().x / 2.f),
                    randfloat(-context.get_world_size().y / 2.f, context.get_world_size().y / 2.f),
                    randfloat(-context.get_world_size().z / 2.f, context.get_world_size().z / 2.f)
                });
                new_agent.set_rotation({
                    randfloat(-1, 1),
                    randfloat(-1, 1),
                    randfloat(-1, 1),
                });
                new_agent.set_time_born(context.get_sim_time());
                context.add_object(new_agent);
            }
        }
    };
}

#endif // SWARMULATOR_CPP_FORAGESOURCE_H
