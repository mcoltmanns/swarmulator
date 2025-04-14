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

class NeuralAgent final : public Agent {
private:
    static constexpr unsigned int num_inputs_ = 12 ;//+ 9; // 2 signals * 6 directions + hidden outputs from last timestep
    static constexpr unsigned int num_outputs_ = 5; // x, y, z steerage + 2 signals
    static constexpr unsigned int num_hidden_ = 9; // average seems good for hidden

    FANN::neural_net brain_ = FANN::neural_net(1, 3, (unsigned int []) { num_inputs_, num_hidden_, num_outputs_ }); // should be LAYER type - each connection has only connections to next layer
    //std::array<float, 9> last_hidden_output_; // memory inputs (outputs of hidden layer from last forward propagation)
    std::array<fann_type, 12> sensory_input_; // sensory inputs
    std::array<fann_type, 2> signal_output_;
    std::array<fann_type, 3> steering_output_;
public:
    NeuralAgent();
    NeuralAgent(const Vector3 position, const Vector3 rotation);
    ~NeuralAgent() override = default;

    void update(const std::vector<Agent *> &neighborhood,
                const std::vector<env::Sphere *> &objects, float dt);

    std::array<fann_type, 2> get_signals() const { return signal_output_; }
    fann_type get_signal(const int idx) const { return signal_output_[idx]; }

    NeuralAgent *mutate(float mutation_chance = 0.5f);
};

} // agent

#endif //NEURALAGENT_H
