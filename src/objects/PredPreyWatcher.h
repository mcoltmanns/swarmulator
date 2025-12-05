//
// Created by moltma on 12/2/25.
//

#ifndef SWARMULATOR_CPP_PREDPREYWATCHER_H
#define SWARMULATOR_CPP_PREDPREYWATCHER_H
#include "../sim/SimObject.h"
#include "../sim/Simulation.h"
#include "Predator.h"
#include "Prey.h"

namespace swarmulator {

    // like PDWatcher, this object is responsible for making sure the simulation doesn't run too low on any one type of simobject
    // plants can't die so it doesn't watch for those, but predators and prey might run out so if they get too low, add more of them
    class PredPreyWatcher final : public SimObject {
    private:
        int min_plants_ = 0;
        int min_pred_ = 0;
        int min_prey_ = 0;
        int miracle_factor_ = 10; // how many more times agents than the minimum to add once the minimum is reached
    public:
        PredPreyWatcher() = default;
        PredPreyWatcher(const Vector3 p, const Vector3 r, const int min_plants, const int min_pred, const int min_prey) : SimObject(p, r), min_plants_(min_plants), min_pred_(min_pred), min_prey_(min_prey) {}
        ~PredPreyWatcher() override = default;

        void update(Simulation &context, const std::list<SimObject*> &neighborhood, float dt) override {
            interaction_radius_ = 0;
            active_ = true;

            if (context.get_object_type_count<Predator>() < min_pred_) {
                for (int i = 0; i < min_pred_ * miracle_factor_; i++) {
                    auto new_agent = Predator();
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
                std::cout << "predator population adjusted" << std::endl;
            }

            if (context.get_object_type_count<Prey>() < min_prey_) {
                for (int i = 0; i < min_prey_ * miracle_factor_; i++) {
                    auto new_agent = Prey();
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
                std::cout << "prey population adjusted" << std::endl;
            }

            if (context.get_object_type_count<Plant>() < min_plants_) {
                auto new_agent = Plant();
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

        [[nodiscard]] std::vector<float> log() const override { return {}; }
        [[nodiscard]] std::vector<float> static_log() const override { return {}; }
        [[nodiscard]] std::string type_name() const override { return "PredPreyWatcher"; }
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_PREDPREYWATCHER_H
