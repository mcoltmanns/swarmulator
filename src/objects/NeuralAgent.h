//
// Created by moltma on 10/22/25.
//

#ifndef SWARMULATOR_CPP_NEURALAGENT_H
#define SWARMULATOR_CPP_NEURALAGENT_H
#include <array>
#include <eigen3/Eigen/Eigen>

#include "../sim/SimObject.h"
#include "../sim/util.h"

namespace swarmulator {
    class NeuralAgent : public SimObject {
    protected:
        // brain shape
        static constexpr unsigned int num_inputs_ = 12; // 6 cardinal dirs, 2 signals from each dir
        static constexpr unsigned int num_hidden_ = 10; // chosen pretty arbitrarily (sort of near the average of 12 and 5)
        static constexpr unsigned int num_outputs_ = 5; // pitch, yaw, signal 0, signal 1, decision (for pd)

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
        float context_weight_ = randfloat(-1.f, 1.f); // weight of context layer (strength with which old hidden layer outputs are piped back in at next runthrough) (should probably not be greater than 1)

        // other params
        static constexpr float initial_energy_ = 2; // how much energy the agents start with
        float energy_ = initial_energy_; // how much energy the agent has
        float reproduction_threshold_ = 10; // how much energy needed to reproduce
        float reproduction_cost_ = 8; // how much energy reproduction costs
        float signal_cost_ = 0.001; // how much energy it costs to output a signal of 1 per unit time (if this is too high, no one engages in signalling)
        float basic_cost_ = 0.01; // how much energy it costs to be alive per unit time
        float move_speed_ = 1; // how many units space you move per unit time (smaller value forces the agent to deal with what is at hand rather than run away)
        size_t parent_id_ = 0;

        float max_lifetime_ = 5000; // how many unit time this agent may be alive for at most
        double time_born_ = 0;

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

        // mutate this agent's weights (as well as its context weight)
        void mutate(float mutation_chance = 0.05); // mutation chance is the probability each weight or bias changes
        // we have 181 parameters, and 5-10 changes per generation are a good target (see stanley's NEAT papers, 2002-2004)
        // 7/181 = x ~= 4% chance for the number of changes we want

    public:
        NeuralAgent();
        NeuralAgent(Vector3 position, Vector3 rotation);
        ~NeuralAgent() override = default;

        [[nodiscard]] auto get_signals() const { return signals_; }

        [[nodiscard]] auto get_energy() const { return energy_; }
        void change_energy(const float amount) { energy_ += amount; }

        void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) override;

        [[nodiscard]] SSBOObject to_ssbo() const override;

        [[nodiscard]] std::string type_name() const override { return "NeuralAgent"; };
        [[nodiscard]] std::vector<float> log() const override;
    };

} // namespace swarmulator

#endif // SWARMULATOR_CPP_NEURALAGENT_H
