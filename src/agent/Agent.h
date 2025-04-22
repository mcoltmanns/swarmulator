//
// Created by moltmanns on 4/2/25.
//

#ifndef AGENT_H
#define AGENT_H

#include <memory>
#include <vector>
#include <vector>


#include "../env/Sphere.h"
#include "raylib.h"

namespace swarmulator::agent {

typedef struct {
    Vector4 position;
    Vector4 direction;
    Vector2 signals; // for communicative agent signals
    Vector2 info; // for additional info (agent type, etc)
} SSBOAgent;

class Agent {
private:
    float cohesion_wt_ = 1;
    float avoidance_wt_ = 1;
    float alignment_wt_ = 1;

protected:
    Vector3 position_ = Vector3(0, 0, 0);
    Vector3 direction_ = Vector3(0, 0, 0);

    float rot_speed_ = 1; // rotation speed in one axle (radians/second)
    float move_speed_ = 0.06;

    float sense_range_ = 0.03;

    bool alive_ = true;

public:
    Agent();
    Agent(const Vector3 position, const Vector3 rotation) : position_(position), direction_(rotation) {};
    virtual ~Agent() = default;

    virtual void update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                        const std::vector<std::shared_ptr<env::Sphere>> &objects, float dt);

    void set_position(const Vector3 position) { position_ = position; }
    void set_direction(const Vector3 rotation) { direction_ = rotation; }

    [[nodiscard]] Vector3 get_position() const { return position_; }
    [[nodiscard]] Vector3 get_direction() const { return direction_; }
    [[nodiscard]] bool is_alive() const { return alive_; }
    [[nodiscard]] bool can_reproduce() const { return false; }

    virtual void to_ssbo(SSBOAgent *out) const;
};

} // swarmulator

#endif //AGENT_H
