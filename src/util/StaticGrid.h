//
// Created by moltmanns on 4/2/25.
// space partitioning grid centered on 0, 0, 0
// sorts agents into cells
//

#ifndef STATICGRID_H
#define STATICGRID_H
#include <cstdint>
#include <memory>
#include <vector>

#include "../agent/Agent.h"
#include "../agent/NeuralAgent.h"
#include "v3ops.h"
#include "raylib.h"


namespace swarmulator::util {

class StaticGrid {
private:
    Vector3 world_size_{};
    Vector3 cell_size_{};
    int axis_cell_count_ = 0;
    int total_cell_count_ = 0;

    std::vector<agent::NeuralAgent *> sorted{};
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

    // sort an array of agents into the grid
    // not so great to put it here but have to for the template to work (must be in same translation unit as declaration)
    template<class T>
    void sort_agents(const std::vector<T *> &in) {
        static_assert(std::is_base_of_v<agent::Agent, T>, "agents must derive from swarmulator::agent::Agent");
        sorted = std::vector<T *>(in.size());
        segment_start = std::vector<uint32_t>(total_cell_count_, 0);
        segment_length = std::vector<uint32_t>(total_cell_count_, 0);

        // count the number of agents in each cell
        for (const auto &agent : in) {
            auto agent_pos = agent->get_position();
            auto pos_grid = agent_pos + 0.5f * world_size_;
            if (const auto cell = cell_index(pos_grid); cell != -1) { // only add agents if they're in bounds
                ++segment_start[cell];
                ++segment_length[cell];
            }
        }

        // compute prefix sum
        // not really worth the effort to parallelize
        for (int i = 1; i <= total_cell_count_; i++) {
            segment_start[i] += segment_start[i - 1];
        }

        // sort agents into their cells
        for (auto it = in.rbegin(); it != in.rend(); ++it) { // careful! need to iterate in reverse here
            const auto agent = *it;
            auto agent_pos = agent->get_position();
            auto pos_grid = agent_pos + 0.5f * world_size_;
            if (const auto cell = cell_index(pos_grid); cell != -1) {
                sorted[--segment_start[cell]] = agent;
            }
        }
    }
    // get all neighbors of a given agent (agents in that agent's cell and neighboring cells)
    // return does not include agent passed
    [[nodiscard]] std::unique_ptr<std::vector<agent::Agent *>> get_neighborhood(const agent::Agent &agent) const;
};

} // util
// swarmulator

#endif //STATICGRID_H
