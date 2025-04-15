//
// Created by moltmanns on 4/2/25.
//

#include "StaticGrid.h"

#include "raymath.h"
#include "v3ops.h"


namespace swarmulator::agent {
    class NeuralAgent;
}

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
        cell_size_ = Vector3(world_size.x / static_cast<float>(subdivisions), world_size.y / static_cast<float>(subdivisions), world_size.z / static_cast<float>(subdivisions));
        axis_cell_count_ = subdivisions;
        total_cell_count_ = subdivisions * subdivisions * subdivisions;
    }



    std::unique_ptr<std::vector<agent::Agent *>> StaticGrid::get_neighborhood(const agent::Agent &agent) const {
        auto neighborhood = std::make_unique<std::vector<agent::Agent *>>();
        const auto agent_pos = agent.get_position();
        const auto agent_pos_grid = agent_pos + 0.5f * world_size_;

        // iterate over every cell in the neighborhood
        // this is not worth parallelizing - at worst 3^3=27 iterations
        for (const auto n_i = neighborhood_indices(agent_pos_grid); const auto neighborhood_cell : n_i) {
            // iterate over every agent in the current neighborhood cell
            for (int i = 0; i < segment_length[neighborhood_cell]; i++) {
                // add the agent to the neighborhood vector if it isn't the agent we're getting the neighborhood of
                auto neighbor = sorted[segment_start[neighborhood_cell] + i];
                if (neighbor != &agent) {
                    neighborhood->push_back(neighbor);
                }
            }
        }

        return neighborhood;
    }
} // swarmulator::util