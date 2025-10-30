//
// Created by moltma on 10/30/25.
//

#ifndef SWARMULATOR_CPP_PDWATCHER_H
#define SWARMULATOR_CPP_PDWATCHER_H
#include "../sim/SimObject.h"
#include "../sim/Simulation.h"
#include "PDAgent.h"

namespace swarmulator {

    class PDWatcher final : public SimObject {
    public:
        PDWatcher() = default;
        PDWatcher(const Vector3 p, const Vector3 r) : SimObject(p, r) {}
        ~PDWatcher() override = default;

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override {
            interaction_radius_ = 0;
            active_ = true;

            // just add more prisoners if we run out
            if (context.get_total_num_objects() < 200) {
                auto new_agent = PDAgent();
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

        std::vector<float> log() const override { return {}; }
        std::vector<float> static_log() const override { return {}; };
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_PDWATCHER_H
