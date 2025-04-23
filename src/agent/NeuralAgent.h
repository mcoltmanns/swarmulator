//
// Created by moltmanns on 4/12/25.
//

#ifndef NEURALAGENT_H
#define NEURALAGENT_H
#include "Agent.h"

#include <array>
#include <floatfann.h> // tell fann to use floats
#include <fann_cpp.h>

namespace swarmulator::agent {

class NeuralAgent : public Agent {
private:
    static constexpr unsigned int num_inputs_ = 12 + 5; // 2 signals * 6 directions + 5 outputs from last timestep
    static constexpr unsigned int num_outputs_ = 5; // x, y, z steerage + 2 signals
    static constexpr unsigned int num_hidden_ = 9; // average seems good for hidden

    FANN::neural_net brain_ = FANN::neural_net(1, 3, (unsigned int []) { num_inputs_, num_hidden_, num_outputs_ }); // should be LAYER type - each connection has only connections to next layer
    //std::array<float, 9> last_hidden_output_; // memory inputs (outputs of hidden layer from last forward propagation)
    std::array<fann_type, 12> sensory_input_; // sensory inputs
    std::array<fann_type, 2> signal_output_;
    std::array<fann_type, 3> steering_output_;
    std::array<fann_type, 5> last_output_;

protected:
    float energy_ = 0.5;
    float reproduction_threshold_ = 0.8;
    float signal_cost_ = 0.05;
    float basic_cost_ = 0.05;
    float reproduction_cost_ = 0.5;

    std::shared_ptr<NeuralAgent> reproduce(float mutation_chance = 0.5f);

public:
    NeuralAgent();
    NeuralAgent(Vector3 position, Vector3 rotation);
    ~NeuralAgent() override = default;

    std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                const std::vector<std::shared_ptr<env::Sphere>> &objects, float dt) override;

    void to_ssbo(SSBOAgent *out) const override;

    [[nodiscard]] std::array<fann_type, 2> get_signals() const { return signal_output_; }
    [[nodiscard]] fann_type get_signal(const int idx) const { return signal_output_[idx]; }
    [[nodiscard]] bool is_alive() const override { return energy_ > 0; }
};

} // agent

#endif //NEURALAGENT_H
