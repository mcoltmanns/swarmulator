//
// Created by moltmanns on 4/12/25.
//

#ifndef NEURALAGENT_H
#define NEURALAGENT_H
#include "Agent.h"

namespace swarmulator::agent {

class NeuralAgent final : public Agent {
public:
    ~NeuralAgent() override = default;
    void update(const std::vector<Agent *> &neighborhood, float dt) override;
};

} // agent

#endif //NEURALAGENT_H
