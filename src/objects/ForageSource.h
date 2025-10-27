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
                    // weight an agent's reward by its inverse distance to the source
                    const auto dist = Vector3Distance(position_, forager->get_position());
                    constexpr float max_gain = 8.f;
                    const float bonus = 1.f /
                        (1.f / max_gain + dist * (context.get_total_num_objects() / 200.f)); // 200 is a really vague object cap (depends wildly on forage source interaction radius, among other things)
                    // aim for 500-700 foragers max, otherwise the scene becomes too crowded to make good observaitons
                    // the most you can make is the max gain, if you are exactly on the source
                    // then it drops off exponentially with distance
                    forager->change_energy(bonus * dt);
                }
            }
        }
    };
}

#endif // SWARMULATOR_CPP_FORAGESOURCE_H
