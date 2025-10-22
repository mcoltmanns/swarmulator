//
// Created by moltmanns on 9/24/25.
//

#ifndef SWARMULATOR_CPP_SIMOBJECT_H
#define SWARMULATOR_CPP_SIMOBJECT_H
#include <list>
#include <memory>
#include <vector>

#include "raylib.h"

namespace swarmulator {
    class Simulation;
    class SimObject {
    protected:
        Vector3 position_ = Vector3(0, 0, 0);
        Vector3 rotation_ = Vector3(0, 0, 0);
        Vector3 velocity_ = Vector3(0, 0, 0);
        Vector3 scale_ = Vector3(1, 1, 1);

        bool active_ = true;

        float interaction_radius_ = 10;

        size_t id_ = 0;

    public:
        // struct for passing simobject info to gpu buffers
        struct SSBOObject {
            Vector4 position;
            Vector4 rotation;
            Vector4 scale;
            Vector4 info;
        };

        SimObject() = default;
        SimObject(const Vector3& position, const Vector3& rotation) : position_(position), rotation_(rotation) {}
        SimObject(const Vector3& position, const Vector3& rotation, const Vector3& scale) : position_(position), rotation_(rotation), scale_(scale) {}
        SimObject(const Vector3& position, const Vector3& rotation, const Vector3& scale, const Vector3& velocity) : position_(position), rotation_(rotation), velocity_(velocity), scale_(scale) {}

        virtual ~SimObject() = default;

        void set_position(const Vector3 position) { position_ = position; }
        [[nodiscard]] Vector3 get_position() const { return position_; }

        void set_rotation(const Vector3 rotation) { rotation_ = rotation; }
        [[nodiscard]] Vector3 get_rotation() const { return rotation_; }

        void set_velocity(const Vector3 velocity) { velocity_ = velocity; }
        [[nodiscard]] Vector3 get_velocity() const { return velocity_; }

        void set_scale(const Vector3 scale) { scale_ = scale; }
        [[nodiscard]] Vector3 get_scale() const { return scale_; }

        void set_interaction_radius(const float radius) { interaction_radius_ = radius; }
        [[nodiscard]] float get_interaction_radius() const { return interaction_radius_; }

        [[nodiscard]] bool active() const { return active_; }
        void activate() { active_ = true; }
        void deactivate() { active_ = false; }

        [[nodiscard]] size_t get_id() const { return id_; }
        void set_id(const size_t id) { id_ = id; }

        // called at every update
        // context is a reference to the running simulation, and should only be used to call add_objects if this simobject needs to add new objects
        virtual void update(Simulation &context, const std::list<SimObject *> &neighborhood, float dt) { }

        [[nodiscard]] virtual SSBOObject to_ssbo() const;

        [[nodiscard]] virtual std::string type_name() const { return "SimObject"; }
        // dynamic object information
        // should include the object id cast to float somewhere, since the logger doesn't track that
        [[nodiscard]] virtual std::vector<float> log() const { return { static_cast<float>(id_), position_.x, position_.y, position_.z, rotation_.x, rotation_.y, rotation_.z}; };
        // static object information
        // object parameters which do not change over time for all objects of this type
        [[nodiscard]] virtual std::vector<float> static_log() const { return { interaction_radius_ }; }
    };
}

#endif //SWARMULATOR_CPP_SIMOBJECT_H