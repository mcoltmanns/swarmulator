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
#include "v3ops.h"
#include "raylib.h"
#include "raymath.h"


namespace swarmulator::util {

template<class T>
class StaticGrid {
static_assert(std::is_base_of_v<agent::Agent, T>, "agents must derive from swarmulator::agent::Agent");
private:
    Vector3 world_size_{};
    Vector3 cell_size_{};
    int axis_cell_count_ = 0;
    int total_cell_count_ = 0;

    std::vector<std::shared_ptr<T>> sorted{};
    std::vector<uint32_t> segment_start{};
    std::vector<uint32_t> segment_length{};

    // get the 1d cell index of a given grid space position
    // returns -1 if the position was out of bounds
    [[nodiscard]] int cell_index(const Vector3 &pos_grid) const {
        if (pos_grid.x < 0 || pos_grid.y < 0 || pos_grid.z < 0
            || pos_grid.x >= world_size_.x || pos_grid.y >= world_size_.y || pos_grid.z >= world_size_.z) {
            return -1;
        }
        const auto [x, y, z] = floorv3(pos_grid / cell_size_);
        const int xpart = axis_cell_count_ * axis_cell_count_ * static_cast<int>(x);
        const int ypart = axis_cell_count_ * static_cast<int>(y);
        const int zpart = static_cast<int>(z);
        return xpart + ypart + zpart;
    }

    // get the 1d cell index of a given grid space position, and the indices of all cells immediately surrounding that cell
    [[nodiscard]] std::vector<int> neighborhood_indices(const Vector3 &pos_grid) const {
        std::vector<int> indices;
        indices.reserve(3 * 3 * 3);
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                    auto offset = Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    if (auto index = cell_index(pos_grid + offset); index != -1) {
                        indices.push_back(index);
                    }
                }
            }
        }
        return indices;
    }

public:
    StaticGrid(const Vector3 world_size, const int subdivisions) : world_size_(world_size), cell_size_(world_size / subdivisions), axis_cell_count_(subdivisions), total_cell_count_(subdivisions * subdivisions * subdivisions) {}
    ~StaticGrid() = default;

    // sort an array of agents into the grid
    // not so great to put it here but have to for the template to work (must be in same translation unit as declaration)
    void sort_agents(const std::list<std::shared_ptr<T>> &in) {
        sorted = std::vector<std::shared_ptr<T>>(in.size());
        segment_start = std::vector<uint32_t>(total_cell_count_, 0);
        segment_length = std::vector<uint32_t>(total_cell_count_, 0);

        // count the number of agents in each cell
        // slow with parallel
        auto it = in.begin();
//#pragma omp parallel private(it)
        {
            for (it = in.begin(); it != in.end(); ++it) {
//#pragma omp single nowait
                {
                    auto agent_pos = (*it)->get_position();
                    auto pos_grid = agent_pos + 0.5f * world_size_;
                    if (const auto cell = cell_index(pos_grid); cell != -1) { // only add agents if they're in bounds
                        ++segment_start[cell];
                        ++segment_length[cell];
                    }
                }
            }
        }

        // compute prefix sum
        // not really worth the effort to parallelize
        for (int i = 1; i <= total_cell_count_; i++) {
            segment_start[i] += segment_start[i - 1];
        }

        // sort agents into their cells
        // slow with parallel
        auto rit = in.rbegin();
//#pragma omp parallel private(rit)
        {
            for (rit = in.rbegin(); rit != in.rend(); ++rit) { // careful! need to iterate in reverse here
//#pragma omp single nowait
                {
                    const auto agent = *rit;
                    auto agent_pos = agent->get_position();
                    auto pos_grid = agent_pos + 0.5f * world_size_;
                    if (const auto cell = cell_index(pos_grid); cell != -1) {
                        sorted[--segment_start[cell]] = agent;
                    }
                }
            }
        }
    }
    // get all neighbors of a given agent (agents in that agent's cell and neighboring cells)
    // return does not include agent passed
    [[nodiscard]] std::unique_ptr<std::vector<std::shared_ptr<agent::Agent>>> get_neighborhood(const agent::Agent &agent) const {
        auto neighborhood = std::make_unique<std::vector<std::shared_ptr<agent::Agent>>>();
        const auto agent_pos = agent.get_position();
        const auto agent_pos_grid = agent_pos + 0.5f * world_size_;

        // iterate over every cell in the neighborhood
        // this is not worth parallelizing - at worst 3^3=27 iterations
        for (const auto n_i = neighborhood_indices(agent_pos_grid); const auto neighborhood_cell : n_i) {
            // iterate over every agent in the current neighborhood cell
            for (int i = 0; i < segment_length[neighborhood_cell]; i++) {
                // add the agent to the neighborhood vector if it isn't the agent we're getting the neighborhood of
                if (auto neighbor = sorted[segment_start[neighborhood_cell] + i]; neighbor.get() != &agent) {
                    neighborhood->push_back(neighbor);
                }
            }
        }

        return neighborhood;
    }
};

} // util
// swarmulator

#endif //STATICGRID_H
