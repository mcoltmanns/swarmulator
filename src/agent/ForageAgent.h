//
// Created by moltmanns on 4/22/25.
//

#ifndef FORAGEAGENT_H
#define FORAGEAGENT_H
#include "NeuralAgent.h"


namespace swarmulator::agent {

class ForageAgent : public NeuralAgent {
private:
    float energy = 1;
    float reproduction_threshold = 0.8;
    float eat_cooldown = 0.5;
};

} // agent
// swarmulator

#endif //FORAGEAGENT_H
