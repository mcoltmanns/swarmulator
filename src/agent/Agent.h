//
// Created by moltmanns on 4/2/25.
//

#ifndef AGENT_H
#define AGENT_H

#include <list>
#include <memory>
#include <vector>


#include "../env/Sphere.h"
#include "raylib.h"

namespace swarmulator::agent {

inline float global_reward_factor = 1;
static float scale = 0.2; // diameter at which the agents are drawn and measured

typedef struct {
    Vector4 position;
    Vector4 direction;
    Vector2 signals; // for communicative agent signals
    Vector2 info; // for additional info (agent type, etc)
} SSBOAgent;

class Agent {
protected:
    Vector3 position_ = Vector3(0, 0, 0);
    Vector3 direction_ = Vector3(0, 0, 0);

    float rot_speed_ = 1; // rotation speed in one axle (radians/second)
    float move_speed_ = 10;

    float sense_radius_ = 5.f;

    size_t id_ = 0;

public:
    Agent() = default;
    Agent(const Vector3 position, const Vector3 rotation) : position_(position), direction_(rotation) {};
    virtual ~Agent() = default;

    virtual std::shared_ptr<Agent> update(const std::vector<std::shared_ptr<Agent>> &neighborhood,
                                          const std::list<std::shared_ptr<env::Sphere>> &objects, float dt);

    void set_position(const Vector3 position) { position_ = position; }
    void set_direction(const Vector3 rotation) { direction_ = rotation; }

    [[nodiscard]] Vector3 get_position() const { return position_; }
    [[nodiscard]] Vector3 get_direction() const { return direction_; }
    [[nodiscard]] virtual bool is_alive() const { return true; }
    [[nodiscard]] size_t get_id() const { return id_; }
    void set_id(const size_t id) { id_ = id; }

    virtual void to_ssbo(SSBOAgent *out) const;
};

} // swarmulator

#endif //AGENT_H
