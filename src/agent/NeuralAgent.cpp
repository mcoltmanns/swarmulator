//
// Created by moltmanns on 4/12/25.
//

#include "NeuralAgent.h"

#include "raymath.h"
#include "../util/util.h"

namespace swarmulator::agent {
NeuralAgent::NeuralAgent() {
    sensory_input_.fill(0);
    signal_output_.fill(0);
    steering_output_.fill(0);
    last_output_.fill(0);

    brain_.randomize_weights(-1, 1);
    brain_.set_activation_function_output(FANN::activation_function_enum::SIGMOID_SYMMETRIC); // range -1 to 1
    brain_.set_activation_function_hidden(FANN::activation_function_enum::SIGMOID_SYMMETRIC);
}

NeuralAgent::NeuralAgent(const Vector3 position, const Vector3 rotation) : Agent(position, rotation) {
    sensory_input_.fill(0);
    signal_output_.fill(0);
    steering_output_.fill(0);
    last_output_.fill(0);

    brain_.randomize_weights(-1, 1);
    brain_.set_activation_function_output(FANN::activation_function_enum::SIGMOID_SYMMETRIC); // range -1 to 1
    brain_.set_activation_function_hidden(FANN::activation_function_enum::SIGMOID_SYMMETRIC);
}

void NeuralAgent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::vector<std::shared_ptr<env::Sphere>> &objects, const float dt) {
    if (energy_ <= 0) {
        alive_ = false;
        return;
    }
    // get information from your neighbors
    for (const auto& n_a: neighborhood) {
        if (n_a == nullptr) {
            continue;
        }
        const auto neighbor = dynamic_cast<NeuralAgent*>(n_a.get());
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
    // copy in the old outputs
    sensory_input_[12] = last_output_[0];
    sensory_input_[13] = last_output_[1];
    sensory_input_[14] = last_output_[2];
    sensory_input_[15] = last_output_[3];
    sensory_input_[16] = last_output_[4];

    // run everything through the network
    const auto out = brain_.run(sensory_input_.data());

    // apply the steering
    const Vector3 steer_dir = {
        out[0],
        out[1],
        out[2]
    };
    // apply the signals
    signal_output_[0] = out[3];
    signal_output_[1] = out[4];
    const float ip = std::exp(-rot_speed_ * dt);
    direction_ = Vector3Lerp(steer_dir, Vector3Normalize(direction_), ip); // rotate
    position_ = position_ + direction_ * move_speed_ * dt; // then move
    energy_ = energy_ - (signal_cost_ * (std::abs(signal_output_[0]) + std::abs(signal_output_[1])) + basic_cost_) * dt; // adjust your energy
}

void NeuralAgent::to_ssbo(SSBOAgent *out) const {
    out->position.x = position_.x;
    out->position.y = position_.y;
    out->position.z = position_.z;
    out->direction.x = direction_.x;
    out->direction.y = direction_.y;
    out->direction.z = direction_.z;
    out->signals.x = signal_output_[0];
    out->signals.y = signal_output_[1];
    out->info.x = 0;
    out->info.y = 0;
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
