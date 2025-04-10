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

#include "agent.h"
#include "raylib.h"


namespace swarmulator::util {

class StaticGrid {
private:
    Vector3 world_size_{};
    Vector3 cell_size_{};
    int axis_cell_count_ = 0;
    int total_cell_count_ = 0;

    //std::vector<agent::Agent> agents_{};
    std::vector<uint32_t> sorted_{};
    std::vector<uint32_t> segment_start_{};
    std::vector<uint32_t> segment_length_{};

    // get the 1d cell index of a given grid space position, and the indices of all cells immediately surrounding that cell
    [[nodiscard]] std::vector<int> neighborhood_indices(const Vector3 &pos_grid) const;

public:
    StaticGrid(Vector3 world_size, int subdivisions);
    ~StaticGrid() = default;

    // get the 1d cell index of a given grid space position
    // returns -1 if the position was out of bounds
    [[nodiscard]] int cell_index(const Vector3 &pos_grid) const;

    //[[nodiscard]] std::vector<agent::Agent> &agents() { return agents_; } // this may actually be better off as a list
    [[nodiscard]] std::vector<uint32_t> &sorted() { return sorted_; }
    [[nodiscard]] const std::vector<uint32_t> &segment_start() { return segment_start_; }
    [[nodiscard]] const std::vector<uint32_t> &segment_length() { return segment_length_; }
    [[nodiscard]] Vector3 cell_size() const { return cell_size_; }

    //uint32_t add_agent(const agent::Agent &agent);
    //void remove_agent(uint32_t agent_id); // expensive!

    // should call this whenever agent positions change! (aka before doing anything that relies on accurate space partitioning)
    void sort_agents(const std::vector<agent::Agent> &in);

    // get all neighbors of a given agent (agents in that agent's cell and neighboring cells)
    // return does not include agent passed
    // this is most useful for serial access - for parallel you need to rely on sorted, segment_start, and segment_length
    //[[nodiscard]] std::unique_ptr<std::vector<uint32_t>> get_neighborhood(uint32_t agent_id) const;
};

} // util
// swarmulator

#endif //STATICGRID_H
