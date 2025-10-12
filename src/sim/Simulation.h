//
// Created by moltmanns on 9/24/25.
//

#ifndef SWARMULATOR_CPP_SIMULATION_H
#define SWARMULATOR_CPP_SIMULATION_H
#include <list>
#include <memory>

#include "rlgl.h"
#include "StaticGrid.h"

namespace swarmulator {
class Simulation {
private:
    Vector3 world_size_ = {1, 1, 1};
    unsigned int grid_divisions_ = 20;
    util::StaticGrid grid_;

    size_t max_objects_ = 2000;
    std::list<std::unique_ptr<SimObject>> objects_; // the simulation owns the objects, so these are unique ptrs
    SSBOObject* objects_ssbo_array_;
    static constexpr size_t objects_ssbo_size_ = 1024 * 1024;
    unsigned int objects_ssbo_id_;

    double total_time_ = 0;

public:
    Simulation() : grid_(world_size_, grid_divisions_) {
        objects_ssbo_array_ = static_cast<SSBOObject *>(RL_CALLOC(objects_ssbo_size_, sizeof(SSBOObject)));
        objects_ssbo_id_ = rlLoadShaderBuffer(objects_ssbo_size_ * sizeof(SSBOObject), objects_ssbo_array_, RL_DYNAMIC_COPY);
    }

    Simulation(Vector3 world_size, int grid_divisions): world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {
        objects_ssbo_array_ = static_cast<SSBOObject *>(RL_CALLOC(objects_ssbo_size_, sizeof(SSBOObject)));
        objects_ssbo_id_ = rlLoadShaderBuffer(objects_ssbo_size_ * sizeof(SSBOObject), objects_ssbo_array_, RL_DYNAMIC_COPY);
    }

    ~Simulation() {
        RL_FREE(objects_ssbo_array_);
    }

    [[nodiscard]] bool can_add_object() const { return  objects_.size() < max_objects_; }
    [[nodiscard]] size_t get_objects_count() const { return objects_.size(); }
    [[nodiscard]] size_t get_max_objects() const { return max_objects_; }

    [[nodiscard]] unsigned int get_objects_ssbo_id() const { return objects_ssbo_id_; }

    [[nodiscard]] double get_total_time() const { return total_time_; }

    void add_object(std::unique_ptr<SimObject> object); // transfers ownership of the object to the simulation

    void update(float dt);
};
} // swarmulator

#endif //SWARMULATOR_CPP_SIMULATION_H