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
    static constexpr unsigned int num_hidden_ = 11;
    static constexpr unsigned int num_outputs_ = 6; // x + y + z steerage + decision + 2 signals

    std::array<fann_type, 2> signals_;

    Eigen::MatrixXf input_ = Eigen::MatrixXf::Random(1, num_inputs_);
    Eigen::MatrixXf hidden_out_ = Eigen::MatrixXf::Random(1, num_hidden_); // output of hidden layer (after activation)
    Eigen::MatrixXf output_ = Eigen::MatrixXf::Random(1, num_outputs_);
    Eigen::MatrixXf w_in_hidden_ = Eigen::MatrixXf::Random(num_inputs_, num_hidden_); // weights from input to hidden
    Eigen::MatrixXf w_hidden_out_ = Eigen::MatrixXf::Random(num_hidden_, num_outputs_); // weights from hidden to out
    float context_weight_ = 0; // weight of context layer (old hidden layer outputs)

protected:
    static constexpr float initial_energy_ = 0.75;
    float energy_ = initial_energy_;
    float reproduction_threshold_ = 0.8;
    float signal_cost_ = 0.001;
    float basic_cost_ = 0.01;
    float reproduction_cost_ = 0.1;
    float sense_radius_ = 0.05;
    float move_speed_ = 0.2;
    float rot_speed_ = 3.14 * 2; // radians/sec

    std::shared_ptr<NeuralAgent> reproduce(float mutation_chance = 0.2f);

    void think(const std::vector<std::shared_ptr<Agent> > &neighborhood);

    static inline constexpr float tanh(const float x) {
        return std::tanh(x);
    }

    void mutate(float mutation_chance);

public:
    NeuralAgent();
    NeuralAgent(Vector3 position, Vector3 rotation, float scale);
    ~NeuralAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                const std::list<std::shared_ptr<env::Sphere>> &objects, float dt) override;

    void to_ssbo(SSBOAgent *out) const override;

    [[nodiscard]] std::array<fann_type, 2> get_signals() const { return signals_; }
    [[nodiscard]] fann_type get_signal(const int idx) const { return signals_[idx]; }
    [[nodiscard]] bool is_alive() const override { return energy_ > 0; }
};

} // agent

#endif //NEURALAGENT_H
