//
// Created by moltmanns on 4/22/25.
//

#ifndef FORAGEAGENT_H
#define FORAGEAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class ForageAgent : public NeuralAgent {
private:
    float eat_cooldown = 0.5;

public:
    ForageAgent();
    ~ForageAgent() override = default;
};

} // agent
// swarmulator

#endif //FORAGEAGENT_H
