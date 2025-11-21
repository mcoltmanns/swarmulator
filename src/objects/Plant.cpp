//
// Created by moltma on 11/21/25
//

#include "Plant.h"

namespace swarmulator {
    Plant::Plant() : SimObject() {

    }

    Plant::Plant(const Vector3 position, const Vector3 rotation) : SimObject(position, rotation) {

    }

    void Plant::update(Simulation &context, const std::list<SimObject *> &neighborhood, const float dt) {
        // just make sure you're not supposed to be dead
        if (energy_ <= 0) {
            deactivate();
            return;
        }
    }

    std::vector<float> Plant::log() const {
        // log output for plant is position, rotation, energy, time born
        return {
            position_.x,
            position_.y,
            position_.z,
            rotation_.x,
            rotation_.y,
            rotation_.z,
            energy_,
            static_cast<float>(time_born_),
        };
    }
}

