//
// Created by moltma on 10/22/25.
//

#ifndef SWARMULATOR_CPP_NEURALAGENT_H
#define SWARMULATOR_CPP_NEURALAGENT_H
#include <array>
#include <eigen3/Eigen/Eigen>

#include "../sim/SimObject.h"

namespace swarmulator {
    class NeuralAgent : public SimObject {
    protected:
        // brain shape
        static constexpr unsigned int num_inputs_ = 12;
        static constexpr unsigned int num_hidden_ = 10;
        static constexpr unsigned int num_outputs_ = 5;

        // signal output array
        std::array<float, 2> signals_ = {0, 0};

        // brain matrices
        // zeroed brain
        Eigen::MatrixXf input_ = Eigen::MatrixXf::Zero(1, num_inputs_); // network input
        Eigen::MatrixXf hidden_out_ = Eigen::MatrixXf::Zero(1, num_hidden_); // output of hidden layer (after activation)
        Eigen::MatrixXf output_ = Eigen::MatrixXf::Zero(1, num_outputs_); // network output (after activation)
        // random weights
        Eigen::MatrixXf w_in_hidden_ = Eigen::MatrixXf::Random(num_inputs_, num_hidden_); // weights from input to hidden
        Eigen::MatrixXf w_hidden_out_ = Eigen::MatrixXf::Random(num_hidden_, num_outputs_); // weights from hidden to out
        // random biases on hidden layer
        Eigen::MatrixXf b_hidden_ = Eigen::MatrixXf::Random(1, num_hidden_);
        float context_weight_ = 0.5; // weight of context layer (strength with which old hidden layer outputs are piped back in at next runthrough) (should probably not be greater than 1)

        // other params
        static constexpr float initial_energy_ = 2; // how much energy the agents start with
        float energy_ = initial_energy_; // how much energy the agent has
        float reproduction_threshold_ = 10; // how much energy needed to reproduce
        float reproduction_cost_ = 8; // how much energy reproduction costs
        float signal_cost_ = 0.001; // how much energy it costs to output a signal of 1 per unit time
        float basic_cost_ = 0.01; // how much energy it costs to be alive per unit time
        float move_speed_ = 5; // how many units space you move per unit time
        size_t parent_id_ = 0;

        float max_lifetime_ = 5000; // how many unit time this agent may be alive for at most

        // run input through the brain and set outputs
        // zero input when done
        void think();

        // several activation functions
        static inline constexpr float tanh(const float x) {
            return std::tanh(x);
        }
        static inline constexpr float sigmoid(const float x) {
            return 1.f / (1.f + std::expf(-x));
        }
        // utility
        static inline constexpr float decrement(const float x) {
            return x - 1;
        }

        // mutate this agent's weights
        void mutate(float mutation_chance = 0.05); // mutation chance is the probability each brain weight or bias gets a random value between -1 and 1 added

    public:
        NeuralAgent();
        NeuralAgent(Vector3 position, Vector3 rotation);
        ~NeuralAgent() override = default;

        [[nodiscard]] auto get_signals() const { return signals_; }

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override;

        [[nodiscard]] SSBOObject to_ssbo() const override;

        [[nodiscard]] std::string type_name() const override { return "NeuralAgent"; };
        [[nodiscard]] std::vector<float> log() const override;
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_NEURALAGENT_H
