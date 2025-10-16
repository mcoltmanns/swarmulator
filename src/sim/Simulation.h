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

    double total_time_ = 0;

public:
    ObjectInstancer object_instancer_;

    Simulation() : grid_(world_size_, grid_divisions_) {
    }

    Simulation(const Vector3 world_size, const int grid_divisions): world_size_(world_size), grid_divisions_(grid_divisions), grid_(world_size, grid_divisions) {
    }

    ~Simulation() = default;

    // get the amount of simulation time passed since start
    [[nodiscard]] double get_total_time() const { return total_time_; }

    void update(float dt);
    void draw_objects(const Matrix &view) const;
};
} // swarmulator

#endif //SWARMULATOR_CPP_SIMULATION_H