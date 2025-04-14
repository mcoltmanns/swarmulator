//
// Created by moltmanns on 4/12/25.
//

#include "NeuralAgent.h"

#include <iostream>

#include "raymath.h"
#include "../util/util.h"

namespace swarmulator::agent {
NeuralAgent::NeuralAgent() {
    sensory_input_.fill(0);
    signal_output_.fill(0);
    steering_output_.fill(0);

    brain_.randomize_weights(-1.f, 1.f);
}

NeuralAgent::NeuralAgent(const Vector3 position, const Vector3 rotation) : Agent(position, rotation) {
    sensory_input_.fill(0);
    signal_output_.fill(0);
    steering_output_.fill(0);

    brain_.randomize_weights(-1.f, 1.f);
}

void NeuralAgent::update(const std::vector<Agent *> &neighborhood, const std::vector<env::Sphere *> &objects, const float dt) {
    // get information from your neighbors
    for (const auto n_a: neighborhood) {
        const auto neighbor = static_cast<NeuralAgent*>(n_a);
        const auto distance = Vector3Distance(position_, neighbor->get_position());
        // if the neighbor is yourself or the neighbor is too far, go to the next
        // neighbor
        if (neighbor == this || distance > sense_range_) {
            continue;
        }
        const float weight = 1.f / (distance * distance); // weight everything by inverse square distance
        const auto neighbor_signals = neighbor->get_signals();
        const auto [diff_x, diff_y, diff_z] = direction_ - Vector3Normalize(neighbor->get_position() - position_);
        // diffs tells us which cardinal segment the neighbor is in
        if (std::abs(diff_x) > std::abs(diff_y) && std::abs(diff_x) > std::abs(diff_z)) {
            if (diff_x > 0) {
                // the neighbor is in front of us
                sensory_input_[0] += weight * neighbor_signals[0];
                sensory_input_[1] += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is behind us
                sensory_input_[2] += weight * neighbor_signals[0];
                sensory_input_[3] += weight * neighbor_signals[1];
            }
        }
        else if (std::abs(diff_y) > std::abs(diff_x) && std::abs(diff_y) > std::abs(diff_z)) {
            if (diff_y > 0) {
                // the neighbor is above us
                sensory_input_[4] += weight * neighbor_signals[0];
                sensory_input_[5] += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is below us
                sensory_input_[6] += weight * neighbor_signals[0];
                sensory_input_[7] += weight * neighbor_signals[1];
            }
        }
        else if (std::abs(diff_z) > std::abs(diff_x) && std::abs(diff_z) > std::abs(diff_y)) {
            if (diff_z > 0) {
                // the neighbor is to our left
                sensory_input_[8] += weight * neighbor_signals[0];
                sensory_input_[9] += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is to our right
                sensory_input_[10] += weight * neighbor_signals[0];
                sensory_input_[11] += weight * neighbor_signals[1];
            }
        }
    }
    // run everything through the network
    const auto opt = brain_.run(sensory_input_.data());
    steering_output_[0] = opt[0];
    steering_output_[1] = opt[1];
    steering_output_[2] = opt[2];
    signal_output_[0] = opt[3];
    signal_output_[1] = opt[4];

    // apply the steering
    const Vector3 steer_dir = {
        steering_output_[0],
        steering_output_[1],
        steering_output_[2]
    };
    const float ip = std::exp(-rot_speed_ * dt);
    direction_ = Vector3Lerp(steer_dir, Vector3Normalize(direction_), ip);
    position_ = position_ + direction_ * 0.06 * dt;
}

// create a new agent, mutate his weights by a given amount
// weights have a mutation_chance chance of being incremented or decremented by a random float between -1 and 1
// weights are clamped to between -1 and 1
NeuralAgent * NeuralAgent::mutate(const float mutation_chance) {
    std::vector<fann_connection> old_conns, new_conns;
    old_conns.resize(brain_.get_total_connections());
    brain_.get_connection_array(old_conns.data());
    for (const auto &[from_neuron, to_neuron, weight]: old_conns) {
        auto new_weight = randfloat() < mutation_chance ? weight + (randfloat() * 2.f - 1.f) : weight;
        new_weight = std::clamp(new_weight, -1.f, 1.f);
        new_conns.emplace_back(from_neuron, to_neuron, new_weight);
    }
    const auto agent = new NeuralAgent();
    agent->brain_ = brain_;
    agent->brain_.set_weight_array(new_conns.data(), new_conns.size());
    return agent;
}
} // agent
