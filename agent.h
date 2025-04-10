//
// Created by moltmanns on 4/2/25.
//

#ifndef AGENT_H
#define AGENT_H

#include "raylib.h"

namespace swarmulator::agent {

    constexpr float rot_speed = 1;
    constexpr float move_speed = 0.12;
    constexpr float sense_range = 0.03;

    typedef struct {
        Vector4 position;
        Vector4 rotation;
        Vector4 signals;
    } Agent;
} // swarmulator::agent

#endif //AGENT_H
