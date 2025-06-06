//
// Created by moltmanns on 4/2/25.
//

#ifndef AGENT_H
#define AGENT_H

#include <list>
#include <memory>
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid//uuid_generators.hpp>


#include "../env/Sphere.h"
#include "raylib.h"

namespace swarmulator::agent {

inline float global_reward_factor = 1;
static float scale = 0.2; // diameter at which the agents are drawn and measured

typedef struct {
    Vector4 position;
    Vector4 direction;
    Vector4 info_a;
    Vector4 info_b;
} SSBOSimObject;

class SimObject {
protected:
    Vector3 position_ = Vector3(0, 0, 0);
    Vector3 direction_ = Vector3(0, 0, 0);

    float rot_speed_ = 5.f; // rotation speed in one axle (should be equal to move speed if you want agent to be able to stay in place)
    float move_speed_ = 2.5f; // move speed (units/second)

    float sense_radius_ = 25.f;

    boost::uuids::uuid id_ = boost::uuids::random_generator()();
    boost::uuids::uuid parent_ = boost::uuids::nil_generator()();

    float time_born_ = 0;

public:
    SimObject() = default;
    SimObject(const Vector3 position, const Vector3 rotation) : position_(position), direction_(rotation) {};
    virtual ~SimObject() = default;

    virtual std::shared_ptr<SimObject> update(const std::vector<std::shared_ptr<SimObject> > &neighborhood,
                                              float dt);

    void set_position(const Vector3 position) { position_ = position; }
    void set_direction(const Vector3 rotation) { direction_ = rotation; }

    [[nodiscard]] Vector3 get_position() const { return position_; }
    [[nodiscard]] Vector3 get_direction() const { return direction_; }

    void set_sense_radius(const float value) { sense_radius_ = value; }
    [[nodiscard]] float get_sense_radius() const { return sense_radius_; }

    [[nodiscard]] virtual bool is_alive() const { return true; }

    [[nodiscard]] boost::uuids::uuid get_id() const { return id_; }
    void set_parent(const boost::uuids::uuid parent) { parent_ = parent; }
    [[nodiscard]] boost::uuids::uuid get_parent() const { return parent_; }

    void set_time_born(const float t) { time_born_ = t; }
    [[nodiscard]] float get_time_born() const { return time_born_; }

    [[nodiscard]] virtual std::string get_genome_string() { return ""; }

    virtual void to_ssbo(SSBOSimObject *out) const;
};

} // swarmulator

#endif //AGENT_H
