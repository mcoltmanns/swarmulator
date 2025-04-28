//
// Created by moltmanns on 4/12/25.
//

#ifndef NEURALAGENT_H
#define NEURALAGENT_H
#include "Agent.h"

#include <array>
#include <floatfann.h> // tell fann to use floats
#include <fann_cpp.h>
#include <eigen3/Eigen/Dense>

#include "../util/util.h"

namespace swarmulator::agent {

class NeuralAgent : public Agent {
private:
    static constexpr unsigned int num_inputs_ = 12; // 2 signals * 6 directions
    static constexpr unsigned int num_hidden_ = 10;
    static constexpr unsigned int num_outputs_ = 5; // pitch + yaw + decision + 2 signals

    std::array<fann_type, 2> signals_;

    Eigen::MatrixXf input_ = Eigen::MatrixXf::Random(1, num_inputs_);
    Eigen::MatrixXf hidden_out_ = Eigen::MatrixXf::Random(1, num_hidden_); // output of hidden layer (after activation)
    Eigen::MatrixXf output_ = Eigen::MatrixXf::Random(1, num_outputs_);
    Eigen::MatrixXf w_in_hidden_ = Eigen::MatrixXf::Random(num_inputs_, num_hidden_); // weights from input to hidden
    Eigen::MatrixXf w_hidden_out_ = Eigen::MatrixXf::Random(num_hidden_, num_outputs_); // weights from hidden to out
    float context_weight_ = 0.2; // weight of context layer (old hidden layer outputs)

protected:
    static constexpr float initial_energy_ = 2;
    float energy_ = initial_energy_;
    float reproduction_threshold_ = 10;
    float signal_cost_ = 0.001;
    float basic_cost_ = 0.01;
    float reproduction_cost_ = 8;

    float max_lifetime_ = 1000; // how many seconds this agent may be alive for at most
    float time_born_ = GetTime();

    void think(const std::vector<std::shared_ptr<Agent> > &neighborhood);

    static inline constexpr float tanh(const float x) {
        return std::tanh(x);
    }

    static inline constexpr float sigmoid(const float x) {
        return 1.f / (1.f + std::expf(-x));
    }

public:
    NeuralAgent();
    NeuralAgent(Vector3 position, Vector3 rotation);
    ~NeuralAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;

    void mutate(float mutation_chance = 0.05); // mutation_chance is the percent chance each gene has to be multiplied by a random value between -1 and 1

    void to_ssbo(SSBOAgent *out) const override;

    [[nodiscard]] std::array<fann_type, 2> get_signals() const { return signals_; }
    [[nodiscard]] fann_type get_signal(const int idx) const { return signals_[idx]; }
    [[nodiscard]] bool is_alive() const override { return energy_ > 0 && GetTime() - time_born_ < max_lifetime_; }

    void set_energy(const float energy) { energy_ = energy; }
    void set_time_born(const float time_born) { time_born_ = time_born; }
};

} // agent

#endif //NEURALAGENT_H
