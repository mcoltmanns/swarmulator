//
// Created by moltmanns on 4/2/25.
//

#include "StaticGrid.h"

#include "raymath.h"
#include "v3ops.h"


namespace swarmulator::util {
    int StaticGrid::cell_index(const Vector3 &pos_grid) const {
        if (pos_grid.x < 0 || pos_grid.x >= world_size_.x
            || pos_grid.y < 0 || pos_grid.y >= world_size_.y
            || pos_grid.z < 0 || pos_grid.z >= world_size_.z) {
            return -1;
        }
        const auto [x, y, z] = floorv3(pos_grid / cell_size_);
        const int xpart = axis_cell_count_ * axis_cell_count_ * static_cast<int>(x);
        const int ypart = axis_cell_count_ * static_cast<int>(y);
        const int zpart = static_cast<int>(z);
        return xpart + ypart + zpart;
    }

    std::vector<int> StaticGrid::neighborhood_indices(const Vector3 &pos_grid) const {
        std::vector<int> indices;
        // these are definitely not worth parallelizing - work is 3^3=27
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

    StaticGrid::StaticGrid(const Vector3 world_size, const int subdivisions) {
        world_size_ = world_size;
        cell_size_ = Vector3(world_size.x / static_cast<float>(subdivisions),
                             world_size.y / static_cast<float>(subdivisions),
                             world_size.z / static_cast<float>(subdivisions));
        axis_cell_count_ = subdivisions;
        total_cell_count_ = subdivisions * subdivisions * subdivisions;
    }

    /*uint32_t StaticGrid::add_agent(const agent::Agent &agent) {
        agents_.push_back(agent);
        return agents_.size() - 1;
    }

    void StaticGrid::remove_agent(const uint32_t agent_id) {
        agents_.erase(agents_.begin() + agent_id);
    }*/

    // sort the agent array
    void StaticGrid::sort_agents(const std::vector<agent::Agent> &in) {
        sorted_ = std::vector<uint32_t>(in.size());
        segment_start_ = std::vector<uint32_t>(total_cell_count_, 0);
        segment_length_ = std::vector<uint32_t>(total_cell_count_, 0);

        // count the number of agents in each cell
        for (int i = 0; i < in.size(); i++) {
            auto pos_grid = xyz(in[i].position) + 0.5f * world_size_;
            const auto cell = cell_index(pos_grid);
            if (cell != -1) { // only add agents if they're in bounds
                ++segment_start_[cell];
                ++segment_length_[cell];
            }
        }

        // compute prefix sum
        // not really worth the effort to parallelize
        for (int i = 1; i <= total_cell_count_; i++) {
            segment_start_[i] += segment_start_[i - 1];
        }

        // sort agents into their cells
        for (int i = in.size() - 1; i >= 0; i--) {
            auto pos_grid = xyz(in[i].position) + 0.5f * world_size_;
            const auto cell = cell_index(pos_grid);
            if (cell != -1) { // only sort agents if they're inbounds
                sorted_[--segment_start_[cell]] = i;
            }
        }
    }

    /*std::unique_ptr<std::vector<uint32_t>> StaticGrid::get_neighborhood(const uint32_t agent_id) const {
        auto neighborhood = std::make_unique<std::vector<uint32_t>>();
        const auto agent_pos_grid = xyz(agents_[agent_id].position) + 0.5f * world_size_;

        // iterate over every cell in the neighborhood
        // this is not worth parallelizing - at worst 3^3=27 iterations
        for (const auto n_i = neighborhood_indices(agent_pos_grid); const auto neighborhood_cell : n_i) {
            // iterate over every agent in the current neighborhood cell
            for (int i = 0; i < segment_length_[neighborhood_cell]; i++) {
                // add the agent to the neighborhood vector if it isn't the agent we're getting the neighborhood of
                if (auto neighbor = sorted_[segment_start_[neighborhood_cell] + i]; neighbor != agent_id) {
                    neighborhood->push_back(neighbor);
                }
            }
        }

        return neighborhood;
    }*/
} // swarmulator::util