//
// Created by moltmanns on 4/12/25.
//

#include "NeuralAgent.h"

#include "raymath.h"

namespace swarmulator::agent {
    void NeuralAgent::update(const std::vector<Agent *> &neighborhood, float dt) {
        for (const auto neighbor : neighborhood) {
            const auto distance = Vector3Distance(position_, neighbor->get_position());
            if (neighbor == this || distance > sense_range_) {
                continue;
            }
            const auto [x, y, z] = direction_ - Vector3Normalize(neighbor->get_position()- position_); // diffs tells us which cardinal segment the neighbor is in
            if (std::abs(x) > std::abs(y) && std::abs(x) > std::abs(z)) {
                if (x > 0) {
                    // the neighbor is in front of us
                }
                else {
                    // the neighbor is behind us
                }
            }
            else if (std::abs(y) > std::abs(x) && std::abs(y) > std::abs(z)) {
                if (y > 0) {
                    // the neighbor is above us
                }
                else {
                    // the neighbor is below us
                }
            }
            else if (std::abs(z) > std::abs(x) && std::abs(z) > std::abs(y)) {
                if (z > 0) {
                    // the neighbor is to our left
                }
                else {
                    // the neighbor is to our right
                }
            }
        }
    }
} // agent