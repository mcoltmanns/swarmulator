//
// Created by moltma on 10/22/25.
//

#include "NeuralAgent.h"

#include "../sim/Simulation.h"
#include "../sim/util.h"
#include "raymath.h"

namespace swarmulator {
    NeuralAgent::NeuralAgent() : SimObject() {
        signals_.fill(0);
    }

    NeuralAgent::NeuralAgent(const Vector3 position, const Vector3 rotation) : SimObject(position, rotation) {
        signals_.fill(0);
    }

    void NeuralAgent::think() {
        // normalize input
        input_.normalize();
        // run the network
        hidden_out_ = (input_ * w_in_hidden_ + hidden_out_ * context_weight_).unaryExpr(&sigmoid) + b_hidden_;
        output_ = (hidden_out_ * w_hidden_out_).unaryExpr(&sigmoid);
        // when you're done thinking, zero your input
        input_.setZero();
    }

    void NeuralAgent::update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) {
        // consider neighbor signals
        for (const auto thing : neighborhood) {
            if (const auto neighbor = dynamic_cast<NeuralAgent*>(thing); neighbor != nullptr) {
                // if the neighbor is another neuralagent, add its signals to the input vector
                const auto dist_sqr = Vector3DistanceSqr(position_, neighbor->get_position());
                const auto wrapped = dist_sqr > interaction_radius_ * interaction_radius_; // if the distance to the other neighbor is further than our interaction radius, that neighbor's position must have been wrapped by the instancer
                const float weight = 1.f / (1.f + dist_sqr); // neurals are weighted by their inverse distance squared
                const auto neighbor_signals = neighbor->get_signals();
                // absolute position difference relative to world axes
                const auto [dx, dy, dz] = neighbor->get_position() - position_;
                // magnitudes of differences tell us which cardinal segment the neighbor is in
                if (std::abs(dx) > std::abs(dy) && std::abs(dx) > std::abs(dz)) {
                    // greatest difference was x, so the neighbor is in front or behind us
                    // this is way nicer than the old code
                    const bool dir = (dx > 0) ^ wrapped; // true is front, back is false
                    const int base = dir ? 0 : 2; // select starting index offset based on direction bool (front inputs are cols 0, 1, back inputs are cols 2, 3)
                    input_(0, base) += weight * neighbor_signals[0];
                    input_(0, base + 1) += weight * neighbor_signals[1];
                }
                else if (std::abs(dy) > std::abs(dx) && std::abs(dy) > std::abs(dz)) {
                    const bool dir = (dy > 0) ^ wrapped; // true is above, false is below
                    const int base = dir ? 4 : 6; // top inputs are cols 4, 5, bottom inputs are cols 6, 7
                    input_(0, base) += weight * neighbor_signals[0];
                    input_(0, base + 1) += weight * neighbor_signals[1];
                }
                else if (std::abs(dz) > std::abs(dx) && std::abs(dz) > std::abs(dy)) {
                    const bool dir = (dz > 0) ^ wrapped; // true is left, false is right
                    const int base = dir ? 8 : 10; // left inputs are cols 8, 9, right inputs are cols 10, 11
                    input_(0, base) += weight * neighbor_signals[0];
                    input_(0, base + 1) += weight * neighbor_signals[1];
                }
            }
            // if you want to do other things with other objects, do them here
        }
        // run the network
        think();
        // network output is between 0 and 1, so scale between -1 and 1 and use that to choose an angle between 0 and 2pi to rotate by
        const float pitch = output_(0, 0) * 2.f * std::numbers::pi; // as in witkowski/ikegami - agents select an angle between 0 and 2pi to steer in
        const float yaw = output_(0, 1) * 2.f * std::numbers::pi; // no tiller steering! direct heading control!
        // apply signals
        signals_[0] = output_(0, 2);
        signals_[1] = output_(0, 3);
        // apply rotations according to tait-bryan convention - always heading/yaw (z), pitch(y) roll(x)
        // omit roll because 2 axes are enough
        // because we do direct control and not tiller steering, start with an x unit vector and rotate that
        rotation_ = Vector3RotateByAxisAngle(Vector3UnitX, Vector3UnitZ, yaw); // apply yaw
        rotation_ = Vector3RotateByAxisAngle(rotation_, Vector3UnitY, pitch); // apply pitch
        rotation_ = Vector3Normalize(rotation_);
        // now move
        position_ = position_ + rotation_ * move_speed_ * dt;
        // update energy
        energy_ -= (signal_cost_ * (std::abs(signals_[0]) + std::abs(signals_[1])) + basic_cost_) * dt;

        // if you can reproduce, do it
        if (energy_ >= reproduction_threshold_) {
            energy_ -= reproduction_cost_;
            auto child = *this;
            child.position_ = {
                (randfloat() * 2.f - 1.f) * context.get_world_size().x,
                (randfloat() * 2.f - 1.f) * context.get_world_size().y,
                (randfloat() * 2.f - 1.f) * context.get_world_size().z
            };
            child.mutate();
            child.parent_id_ = id_;
            child.time_born_ = context.get_sim_time();
            context.add_object(child);
        }
        // if you died or exceeded max lifetime, deactivate yourself (will be removed at next update)
        else if (energy_ <= 0 || context.get_sim_time() - time_born_ > max_lifetime_) {
            deactivate();
        }
    }

    void NeuralAgent::mutate(const float mutation_chance) {
        for (int i = 0; i < swarmulator::NeuralAgent::num_hidden_; i++) {
            for (int j = 0; j < swarmulator::NeuralAgent::num_inputs_; j++) {
                w_in_hidden_(j, i) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
            }
            for (int k = 0; k < swarmulator::NeuralAgent::num_outputs_; k++) {
                w_hidden_out_(i, k) += randfloat() < mutation_chance ? randfloat() * 2.f - 1.f : 0;
            }
            b_hidden_(0, i) += randfloat() < mutation_chance ? 0 : randfloat() * 2.f - 1.f;
        }
    }

    SimObject::SSBOObject NeuralAgent::to_ssbo() const {
        SSBOObject out;
        out.position.w = 0;
        out.position.x = position_.x;
        out.position.y = position_.y;
        out.position.z = position_.z;

        out.rotation.w = 0;
        out.rotation.x = rotation_.x;
        out.rotation.y = rotation_.y;
        out.rotation.z = rotation_.z;

        out.info.w = 0;
        out.info.x = signals_[0];
        out.info.y = signals_[1];
        out.info.z = 0;

        out.scale = Vector4(1, 1, 1, 1);

        return out;
    }

    std::vector<float> NeuralAgent::log() const {
        std::vector out = {
            static_cast<float>(id_),
            static_cast<float>(parent_id_),
            position_.x,
            position_.y,
            position_.z,
            rotation_.x,
            rotation_.y,
            rotation_.z,
        };

        // weight matrices are flattened column-major
        // a b
        // c d
        // becomes a c b d
        // push back in-hidden weights
        const float* flat = w_in_hidden_.data();
        for (int i = 0; i < w_in_hidden_.size(); i++) {
            out.push_back(flat[i]);
        }
        // push back hidden-out weights
        flat = w_hidden_out_.data();
        for (int i = 0; i < w_hidden_out_.size(); i++) {
            out.push_back(flat[i]);
        }
        // push back biases
        flat = b_hidden_.data();
        for (int i = 0; i < b_hidden_.size(); i++) {
            out.push_back(flat[i]);
        }

        return out;
    }
} // namespace swarmulator
