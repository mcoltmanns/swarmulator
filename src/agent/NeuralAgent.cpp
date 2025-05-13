//
// Created by moltmanns on 4/12/25.
//

#include "NeuralAgent.h"

#include <iostream>

#include "raymath.h"
#include "../util/util.h"

namespace swarmulator::agent {
NeuralAgent::NeuralAgent() : Agent() {
    signals_.fill(0);
    w_in_hidden_.normalize();
    w_hidden_out_.normalize();
}

NeuralAgent::NeuralAgent(const Vector3 position, const Vector3 rotation) : Agent(position, rotation) {
    signals_.fill(0);
    w_in_hidden_.normalize();
    w_hidden_out_.normalize();
}

void NeuralAgent::think(const std::vector<std::shared_ptr<Agent> > &neighborhood) {
    input_.setZero(); // zero your input!
    // get information from your neighbors
    for (const auto& n_a: neighborhood) {
        const auto neighbor = dynamic_cast<NeuralAgent*>(n_a.get());
        const auto dist_sqr = Vector3DistanceSqr(position_, neighbor->get_position());
        // if the neighbor is yourself or the neighbor is too far, go to the next
        // neighbor
        if (neighbor == this || dist_sqr > sense_radius_ * sense_radius_) {
            continue;
        }
        const float weight = 1.f / (1.f + dist_sqr); // weight everything by inverse square distance
        const auto neighbor_signals = neighbor->get_signals();
        // this is the absolute difference - not rotated by the direction vector!
        const auto [diff_x, diff_y, diff_z] = /*direction_ -*/ neighbor->get_position() - position_;
        // diffs tells us which cardinal segment the neighbor is in
        // greatest magnitude is x
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
        // greatest magintude is y
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
        // greatest magnitude is z
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
    // normalize before we run
    //const auto norm = input_.norm();
    //input_ = norm != 0 ? input_ * (1.f / input_.norm()) : input_;
    // run everything through the network
    hidden_out_ = (input_ * w_in_hidden_ + hidden_out_ * context_weight_).unaryExpr(&sigmoid) + b_hidden_;
    output_ = (hidden_out_ * w_hidden_out_).unaryExpr(&sigmoid);
}

std::shared_ptr<Agent> NeuralAgent::update(const std::vector<std::shared_ptr<Agent>> &neighborhood, const std::list<std::shared_ptr<env::Sphere>> &objects, const float dt) {
    think(neighborhood);
    // remember output is between 0 and 1 - so we scale to between -1 and 1, and then use that to choose an angle between -2pi and 2pi to rotate by
    const float pitch = ((output_(0, 0) - 0.5) * 2.f) * std::numbers::pi * 2.f * rot_speed_ * dt; // rotation about world y axis (elevation angle/psi) (control direction z part)
    const float yaw = ((output_(0, 1) - 0.5) * 2.f) * std::numbers::pi * 2.f * rot_speed_ * dt; // rotation about world z axis (bearing/theta) (control direction x and y part)
    // apply the signals
    signals_[0] = output_(0, 2);
    signals_[1] = output_(0, 3);
    // apply
    direction_ = Vector3RotateByAxisAngle(direction_, Vector3UnitY, pitch); // appy pitch
    direction_ = Vector3RotateByAxisAngle(direction_, Vector3UnitZ, yaw); // then yaw
    position_ = position_ + direction_ * move_speed_ * dt; // then move
    energy_ -= (signal_cost_ * (std::abs(signals_[0]) + std::abs(signals_[1])) + basic_cost_) * dt; // adjust your energy
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

std::string NeuralAgent::get_genome_string() {
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < num_inputs_; i++) {
        for (int j = 0; j < num_hidden_; j++) {
            ss << w_in_hidden_(i, j) << ", ";
        }
    }
    for (int i = 0; i < num_hidden_; i++) {
        for (int j = 0; j < num_outputs_; j++) {
            ss << w_hidden_out_(i, j) << ", ";
        }
    }
    for (int i = 0; i < num_hidden_ - 1; i++) {
        ss << b_hidden_(0, i) << ", ";
    }
    ss << b_hidden_(0, num_hidden_ - 1) << "]";
    return ss.str();
}

void NeuralAgent::mutate(const float mutation_chance) {
    for (int i = 0; i < num_hidden_; i++) {
        for (int j = 0; j < num_inputs_; j++) {
            w_in_hidden_(j, i) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
        }
        for (int k = 0; k < num_outputs_; k++) {
            w_hidden_out_(i, k) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
        }
        b_hidden_(0, i) += randfloat() < mutation_chance ? randfloat() * 2 - 1.f : 0;
    }
    /*// move everything back to range 0, 1
    w_in_hidden_.normalize();
    w_hidden_out_.normalize();
    b_hidden_.normalize();
    // move everything to range 0, 2
    w_in_hidden_ *= 2.f;
    w_hidden_out_ *= 2.f;
    b_hidden_ *= 2.f;
    // move everything to range -1, 1
    w_in_hidden_ = w_in_hidden_.unaryExpr(&decrement);
    w_hidden_out_ = w_hidden_out_.unaryExpr(&decrement);
    b_hidden_ = b_hidden_.unaryExpr(&decrement);*/
}
} // agent
