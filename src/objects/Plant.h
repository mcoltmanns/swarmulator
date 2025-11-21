//
// Created by moltma on 11/21/25.
//

#ifndef SWARMULATOR_CPP_PLANT_H
#define SWARMULATOR_CPP_PLANT_H

#include "../sim/SimObject.h"
#include "../sim/util.h"

namespace swarmulator {
    class Plant : public SimObject {
        // plants are very simple
        // they just sit around waiting to be eaten
    protected:
        static constexpr float initial_energy_ = 10;
        float energy_ = initial_energy_;
        double time_born_ = 0;

    public:
        Plant();
        Plant(Vector3 position, Vector3 rotation);
        ~Plant() override = default;

        [[nodiscard]] auto get_energy() const { return energy_; }
        void change_energy(const float amount) { energy_ += amount; }

        // plants don't do much
        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override {};

        [[nodiscard]] std::string type_name() const override { return "Plant"; };
        [[nodiscard]] std::vector<float> log() const override;

        void set_time_born(const double time) { time_born_ = time ; }
    };
}

#endif

