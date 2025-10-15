//
// Created by moltmanns on 9/24/25.
//

#ifndef SWARMULATOR_CPP_SIMULATION_H
#define SWARMULATOR_CPP_SIMULATION_H
#include <memory>

#include "ObjectInstancer.h"
#include "StaticGrid.h"

namespace swarmulator {
class Simulation {
private:
    Vector3 world_size_ = {1, 1, 1};
    int grid_divisions_ = 20;
    // simulation gets a grid
    StaticGrid grid_;
    // simulation gets an object instancer
    ObjectInstancer object_instancer_;

    double total_time_ = 0;

public:

    Simulation() : grid_(world_size_, grid_divisions_) {
    }

    Simulation(const Vector3 world_size, const int grid_divisions): world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {
    }

    ~Simulation() = default;

    [[nodiscard]] double get_total_time() const { return total_time_; }
    [[nodiscard]] size_t get_objects_count() const { return object_instancer_.size(); }
    template<class T>
    [[nodiscard]] size_t get_objects_count() const {
        const auto& grp = object_instancer_.get_object_group<T>();
        return grp.objects.size();
    }

    // all to do here is expose the methods from the instancer that we want
    // the only thing we actually need is adding objects for the initial setup
    template<class T>
    void add_object(std::shared_ptr<T> object) { object_instancer_.add_object<T>(object); } // object should be allocated with new, will be freed when simulation ends or object is removed, whatever

    template<class T>
    size_t alloc_object_type(const std::string& vtx_src_p, const std::string& frg_src_p, const Mesh& mesh) { return object_instancer_.alloc_object_group<T>(vtx_src_p, frg_src_p, mesh); };

    void update(float dt);
    void draw_objects(const Matrix &projection, const Matrix &view);
};
} // swarmulator

#endif //SWARMULATOR_CPP_SIMULATION_H