//
// Created by moltmanns on 4/12/25.
//

#ifndef NEURALAGENT_H
#define NEURALAGENT_H
#include "Agent.h"

#include <array>
#include <eigen3/Eigen/Dense>

#include "../util/util.h"

namespace swarmulator::agent {

class NeuralAgent : public Agent {
protected:
    static constexpr unsigned int num_inputs_ = 12; // 2 signals * 6 directions
    static constexpr unsigned int num_hidden_ = 10;
    static constexpr unsigned int num_outputs_ = 5; // pitch + yaw + decision + 2 signals

    std::array<float, 2> signals_ = {0, 0};

    // zeroed network
    Eigen::MatrixXf input_ = Eigen::MatrixXf::Zero(1, num_inputs_);
    Eigen::MatrixXf hidden_out_ = Eigen::MatrixXf::Zero(1, num_hidden_); // output of hidden layer (after activation)
    Eigen::MatrixXf output_ = Eigen::MatrixXf::Zero(1, num_outputs_);
    // random weights
    Eigen::MatrixXf w_in_hidden_ = Eigen::MatrixXf::Random(num_inputs_, num_hidden_); // weights from input to hidden
    Eigen::MatrixXf w_hidden_out_ = Eigen::MatrixXf::Random(num_hidden_, num_outputs_); // weights from hidden to out
    // random biases on hidden layer
    Eigen::MatrixXf b_hidden_ = Eigen::MatrixXf::Random(1, num_hidden_);
    float context_weight_ = 0.5f; // weight of context layer (old hidden layer outputs) (this should probably be between 0 and 1)

protected:
    static constexpr float initial_energy_ = 2;
    float energy_ = initial_energy_;
    float reproduction_threshold_ = 10;
    float signal_cost_ = 0.001; // how much energy it costs to give a signal of 1 per second
    float basic_cost_ = 0.01; // how much energy you lose by default per second
    float reproduction_cost_ = 8;

    float max_lifetime_ = 1000; // how many updates this agent may be alive for at most

    void think(const std::vector<std::shared_ptr<Agent> > &neighborhood);

    static inline constexpr float tanh(const float x) {
        return std::tanh(x);
    }

    static inline constexpr float sigmoid(const float x) {
        return 1.f / (1.f + std::expf(-x));
    }

    static inline constexpr float decrement(const float x) {
        return x - 1;
    }

public:
    NeuralAgent();
    NeuralAgent(Vector3 position, Vector3 rotation);
    ~NeuralAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;

    void mutate(float mutation_chance = 0.05); // mutation_chance is the percent chance each gene component (weight or bias) has to have a random value between -1 and 1 added

    void to_ssbo(SSBOAgent *out) const override;

    [[nodiscard]] std::array<float, 2> get_signals() const { return signals_; }
    [[nodiscard]] float get_signal(const int idx) const { return signals_[idx]; }
    [[nodiscard]] bool is_alive() const override { return energy_ > 0 && GetTime() - time_born_ < max_lifetime_; }

    void set_energy(const float energy) { energy_ = energy; }
    [[nodiscard]] float get_energy() const { return energy_; }

    [[nodiscard]] std::string get_genome_string() override;
};

} // agent

#endif //NEURALAGENT_H
