//
// Created by moltmanns on 9/24/25.
//

#ifndef SWARMULATOR_CPP_SIMOBJECT_H
#define SWARMULATOR_CPP_SIMOBJECT_H
#include <memory>
#include <vector>

#include "dtypes.h"

namespace swarmulator {
    class SimObject {
    protected:
        Vector3 position_ = Vector3(0, 0, 0);
        Vector3 rotation_ = Vector3(0, 0, 0); // TODO move to quaternions maybe in future
        Vector3 velocity_ = Vector3(0, 0, 0);
        Vector3 scale_ = Vector3(1, 1, 1);

        bool active_ = true;

        float interaction_radius_ = 10;

    public:
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

        // called at every update
        virtual void update(const std::vector<SimObject*> &neighborhood, float dt) {}

        SSBOObject to_ssbo() const;
    };
}

#endif //SWARMULATOR_CPP_SIMOBJECT_H