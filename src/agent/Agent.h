//
// Created by moltmanns on 4/2/25.
//

#ifndef AGENT_H
#define AGENT_H
#include <vector>


#include "raylib.h"

namespace swarmulator::agent {

class Agent {
protected:
    Vector3 position_ = Vector3(0, 0, 0);
    Vector3 direction_ = Vector3(0, 0, 0);

    float rot_speed_ = 1; // rotation speed in one axle (radians/second)

    float sense_range_ = 0.03;
    float cohesion_wt_ = 1;
    float avoidance_wt_ = 1;
    float alignment_wt_ = 1;

    // vector of sensors (6 - one in each direction)
    // single actuator
    static unsigned int vao_;

public:
    Agent();
    Agent(const Vector3 position, const Vector3 rotation) : position_(position), direction_(rotation) {};
    virtual ~Agent() = default;

    virtual void update(const std::vector<Agent *> &neighborhood, float dt);

    void set_position(const Vector3 position) { position_ = position; }
    void set_direction(const Vector3 rotation) { direction_ = rotation; }

    [[nodiscard]] Vector3 get_position() const { return position_; }
    [[nodiscard]] Vector3 get_direction() const { return direction_; }

    [[nodiscard]] static unsigned int get_vao() { return vao_; }
    static void init_vao();
};

} // swarmulator

#endif //AGENT_H
