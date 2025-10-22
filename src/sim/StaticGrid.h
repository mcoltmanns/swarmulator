//
// Created by moltmanns on 4/2/25.
// space partitioning grid centered on 0, 0, 0
// sorts agents into cells
//

#ifndef STATICGRID_H
#define STATICGRID_H
#include <memory>
#include <vector>

#include "ObjectInstancer.h"
#include "SimObject.h"
#include "raylib.h"
#include "raymath.h"
#include "util.h"


namespace swarmulator {

class StaticGrid {
private:
    Vector3 world_size_{};
    Vector3 cell_size_{};
    int axis_cell_count_ = 0; // although these are indexes they should stay int because we represent errors in indexing with -1
    int total_cell_count_ = 0;

    std::vector<SimObject*> sorted {};
    std::vector<uint32_t> segment_start {};
    std::vector<uint32_t> segment_length {};

    // get the 1d cell index of a given grid space position
    // if the position was out of bounds, wrap it
    [[nodiscard]] int cell_index(Vector3 pos_grid) const;

    // get the 1d cell index of a given grid space position, and the indices of all cells surrounding that cell in a given radius
    [[nodiscard]] std::vector<int> neighborhood_indices(const Vector3 &pos_grid, float neighborhood_radius) const;

    // get the 1d cell index of a given grid space position, and the indices of all cells immediately surrounding that cell
    [[nodiscard]] std::vector<int> neighborhood_indices(const Vector3 &pos_grid) const;

public:
    StaticGrid(const Vector3 world_size, const size_t subdivisions) : world_size_(world_size), cell_size_(world_size / static_cast<double>(subdivisions)), axis_cell_count_(subdivisions), total_cell_count_(subdivisions * subdivisions * subdivisions) {}
    ~StaticGrid() = default;

    // sort all objects in an objectinstancer into the grid
    void sort_objects(ObjectInstancer &in);

    // get all neighbors of a given object (objects within that object's interaction radius)
    // return does not include object passed
    [[nodiscard]] std::list<SimObject *> get_neighborhood(const SimObject *object) const;

    // wrap a global position
    [[nodiscard]] Vector3 wrap_position(const Vector3 position) const {
        return swarmulator::wrap_position(position, world_size_);
    }

    void bounce_agent(const std::shared_ptr<SimObject> &agent) const {
        if (agent->get_position().x < -world_size_.x / 2 || agent->get_position().x >= world_size_.x / 2
            || agent->get_position().y < -world_size_.y / 2 || agent->get_position().y >= world_size_.y / 2
            || agent->get_position().z < -world_size_.z / 2 || agent->get_position().z >= world_size_.z / 2) {
            agent->set_rotation(agent->get_rotation() - 0.5f * agent->get_position());
            }
    }
};

} // util
// swarmulator

#endif //STATICGRID_H
