//
// Created by moltmanns on 4/2/25.
// space partitioning grid centered on 0, 0, 0
// sorts agents into cells
//

#ifndef STATICGRID_H
#define STATICGRID_H
#include <cstdint>
#include <vector>
#include <memory>

#include "Agent.h"
#include "raylib.h"


namespace swarmulator::util {

class StaticGrid {
private:
    Vector3 world_size_{};
    Vector3 cell_size_{};
    int axis_cell_count_ = 0;
    int total_cell_count_ = 0;

    std::vector<Agent *> sorted{};
    std::vector<uint32_t> segment_start{};
    std::vector<uint32_t> segment_length{};

    // get the 1d cell index of a given grid space position
    // returns -1 if the position was out of bounds
    [[nodiscard]] int cell_index(const Vector3 &pos_grid) const;

    // get the 1d cell index of a given grid space position, and the indices of all cells immediately surrounding that cell
    [[nodiscard]] std::vector<int> neighborhood_indices(const Vector3 &pos_grid) const;

public:
    StaticGrid(Vector3 world_size, int subdivisions);
    ~StaticGrid() = default;

    void sort_agents(const std::vector<Agent *> &in);
    // get all neighbors of a given agent (agents in that agent's cell and neighboring cells)
    // return does not include agent passed
    [[nodiscard]] std::unique_ptr<std::vector<Agent *>> get_neighborhood(const Agent &agent) const;
};

} // util
// swarmulator

#endif //STATICGRID_H
