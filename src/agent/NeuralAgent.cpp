//
// Created by moltmanns on 4/12/25.
//

#include "NeuralAgent.h"

#include <iostream>

#include "raymath.h"
#include "../util/util.h"

namespace swarmulator::agent {
NeuralAgent::NeuralAgent() : Agent() {
    //input_.fill(0);
    signals_.fill(0);
}

NeuralAgent::NeuralAgent(const Vector3 position, const Vector3 rotation, const float scale) : Agent(position, rotation, scale) {
    //input_.fill(0);
    signals_.fill(0);
}

void NeuralAgent::think(const std::vector<std::shared_ptr<Agent> > &neighborhood) {
    // get information from your neighbors
    for (const auto& n_a: neighborhood) {
        const auto neighbor = dynamic_cast<NeuralAgent*>(n_a.get());
        const auto dist_sqr = Vector3DistanceSqr(position_, neighbor->get_position());
        // if the neighbor is yourself or the neighbor is too far, go to the next
        // neighbor
        if (neighbor == this || dist_sqr > sense_radius_ * sense_radius_) {
            continue;
        }
        const float weight = 1.f / dist_sqr; // weight everything by inverse square distance
        const auto neighbor_signals = neighbor->get_signals();
        const auto [diff_x, diff_y, diff_z] = direction_ - Vector3Normalize(neighbor->get_position() - position_);
        // diffs tells us which cardinal segment the neighbor is in
        if (std::abs(diff_x) > std::abs(diff_y) && std::abs(diff_x) > std::abs(diff_z)) {
            if (diff_x > 0) {
                // the neighbor is in front of us
                input_(0, 0) += weight * neighbor_signals[0];
                input_(0, 1) += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is behind us
                input_(0, 2) += weight * neighbor_signals[0];
                input_(0, 3) += weight * neighbor_signals[1];
            }
        }
        else if (std::abs(diff_y) > std::abs(diff_x) && std::abs(diff_y) > std::abs(diff_z)) {
            if (diff_y > 0) {
                // the neighbor is above us
                input_(0, 4) += weight * neighbor_signals[0];
                input_(0, 5) += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is below us
                input_(0, 6) += weight * neighbor_signals[0];
                input_(0, 7) += weight * neighbor_signals[1];
            }
        }
        else if (std::abs(diff_z) > std::abs(diff_x) && std::abs(diff_z) > std::abs(diff_y)) {
            if (diff_z > 0) {
                // the neighbor is to our left
                input_(0, 8) += weight * neighbor_signals[0];
                input_(0, 9) += weight * neighbor_signals[1];
            }
            else {
                // the neighbor is to our right
                input_(0, 10) += weight * neighbor_signals[0];
                input_(0, 11) += weight * neighbor_signals[1];
            }
        }
    }

/*#pragma omp critical
    {
        std::cout << input_ << std::endl;
    }*/

    // run everything through the network
    hidden_out_ = (input_ * w_in_hidden_ + context_weight_ * hidden_out_).unaryExpr(&tanh);
    output_ = (hidden_out_ * w_hidden_out_).unaryExpr(&sigmoid);
}

std::shared_ptr<Agent> NeuralAgent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, const float dt) {
    //const auto outputs = think(neighborhood); // make a decision based on your neighbors
    think(neighborhood);
    const float pitch = output_(0, 0) * std::numbers::pi * 2.f; // rotation about world y axis (elevation angle/psi) (control direction z part)
    const float yaw = output_(0, 1) * std::numbers::pi * 2.f; // rotation about world z axis (bearing/theta) (control direction x and y part)
    // apply the signals
    signals_[0] = output_(0, 2);
    signals_[1] = output_(0, 3);
    auto decision = output_(0, 4);
    // apply
    direction_ = Vector3Normalize(direction_ + Vector3Normalize(Vector3(std::cos(yaw), std::sin(yaw), std::sin(pitch)) * rot_speed_ * dt)); // TODO does this make sense?
    position_ = position_ + direction_ * move_speed_ * dt; // then move
    energy_ = energy_ - (signal_cost_ * (std::abs(signals_[0]) + std::abs(signals_[1])) + basic_cost_) * dt; // adjust your energy
    // default neuralagent never reproduces
    return nullptr;
}

void NeuralAgent::to_ssbo(SSBOAgent *out) const {
    out->position.x = position_.x;
    out->position.y = position_.y;
    out->position.z = position_.z;
    out->direction.x = direction_.x;
    out->direction.y = direction_.y;
    out->direction.z = direction_.z;
    out->signals.x = signals_[0];
    out->signals.y = signals_[1];
    out->info.x = 0;
    out->info.y = 0;
}

// create a new agent, mutate his weights by a given amount
// weights have a mutation_chance chance of being incremented or decremented by a random float between -1 and 1
// weights are clamped to between -1 and 1
std::shared_ptr<NeuralAgent> NeuralAgent::reproduce(const float mutation_chance) {
    energy_ -= reproduction_cost_; // decrease your energy by the cost of reproduction
    auto agent = std::make_shared<NeuralAgent>(*this); // make a new agent as a copy of this
    agent->energy_ = initial_energy_; // reset its energy
    agent->mutate(mutation_chance); // mutate the new agent
    return agent;
}

void NeuralAgent::mutate(const float mutation_chance) {
    for (int i = 0; i < num_hidden_; i++) {
        for (int j = 0; j < num_inputs_; j++) {
            w_in_hidden_(j, i) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
        }
        for (int k = 0; k < num_outputs_; k++) {
            w_hidden_out_(i, k) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
        }
    }
}
} // agent
