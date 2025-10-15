//
// Created by moltmanns on 9/24/25.
//

#include "StaticGrid.h"

namespace swarmulator {
    [[nodiscard]] int StaticGrid::cell_index(const Vector3 pos_grid) const {
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

    [[nodiscard]] std::vector<int> StaticGrid::neighborhood_indices(const Vector3 &pos_grid, const float neighborhood_radius) const {
        std::vector<int> indices;
        const int nri = static_cast<int>(neighborhood_radius);
        const int csix = static_cast<int>(cell_size_.x);
        const int csiy = static_cast<int>(cell_size_.y);
        const int csiz = static_cast<int>(cell_size_.z);
        // offsets are pos_grid - radius, pos_grid + radius
        for (int x = -nri; x <= nri; x += csix) {
            for (int y = -nri; y <= nri; y += csiy) {
                for (int z = -nri; z <= nri; z += csiz) {
                    auto offset = Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    if (auto index = cell_index(pos_grid + offset); index != -1) {
                        indices.push_back(index);
                    }
                }
            }
        }
        return indices;
    }

    [[nodiscard]] std::vector<int> StaticGrid::neighborhood_indices(const Vector3 &pos_grid) const {
        std::vector<int> indices;
        indices.reserve(3 * 3 * 3);
        const int csix = static_cast<int>(cell_size_.x);
        const int csiy = static_cast<int>(cell_size_.y);
        const int csiz = static_cast<int>(cell_size_.z);
        // KILL BUG!!!!! (see williamdeneen.com) (bug is killed, this is the gravestone)
        for (int x = -csix; x <= csix; x += csix) {
            for (int y = -csiy; y <= csiy; y += csiy) {
                for (int z = -csiz; z <= csiz; z += csiz) {
                    auto offset = Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    if (auto index = cell_index(pos_grid + offset); index != -1) {
                        indices.push_back(index);
                    }
                }
            }
        }
        return indices;
    }

    void StaticGrid::sort_objects(ObjectInstancer &in) {
        sorted = std::vector<std::shared_ptr<SimObject>>(in.size());
        segment_start = std::vector<uint32_t>(total_cell_count_, 0);
        segment_length = std::vector<uint32_t>(total_cell_count_, 0);

        // count the number of agents in each cell
        // slow with parallel
        //auto it = in.begin();
        //#pragma omp parallel private(it)
        {
            for (auto grp = in.groups_begin(); grp != in.groups_end(); ++grp) {
                //#pragma omp single nowait
                {
                    for (auto it = grp->second.objects.begin(); it != grp->second.objects.end(); ++it) {
                        const auto pos_grid = it.operator*()->get_position() + 0.5f * world_size_; // so then this is ok because all we do is look at the memory? ownership of the object is never transferred
                        if (const auto cell = cell_index(pos_grid); cell != -1) { // only add agents if they're in bounds
                            ++segment_start[cell];
                            ++segment_length[cell];
                        }
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
        //auto rit = in.rbegin();
        //#pragma omp parallel private(rit)
        {
            for (auto rgrp = in.groups_rbegin(); rgrp != in.groups_rend(); ++rgrp) { // careful! need to iterate in reverse here
                //#pragma omp single nowait
                {
                    for (auto rit = rgrp->second.objects.rbegin(); rit != rgrp->second.objects.rend(); ++rit) {
                        const auto object = *rit;
                        auto pos = object->get_position();
                        const auto pos_grid = pos + 0.5f * world_size_;
                        if (const auto cell = cell_index(pos_grid); cell != -1) {
                            sorted[--segment_start[cell]] = object;
                        }
                    }
                }
            }
        }
    }

    std::unique_ptr<std::vector<std::shared_ptr<SimObject>>> StaticGrid::get_neighborhood(const SimObject &object,
        const float radius) const {
        auto neighborhood = std::make_unique<std::vector<std::shared_ptr<SimObject>>>();
        const auto object_pos = object.get_position();
        const auto object_pos_grid = object_pos + 0.5f * world_size_;

        // iterate over every cell in the neighborhood
        // this is not worth parallelizing - at worst 3^3=27 iterations
        for (const auto n_i = neighborhood_indices(object_pos_grid, radius); const auto neighborhood_cell : n_i) {
            // iterate over every agent in the current neighborhood cell
            for (int i = 0; i < segment_length[neighborhood_cell]; i++) {
                // add the agent to the neighborhood vector if it isn't the agent we're getting the neighborhood of
                if (auto neighbor = sorted[segment_start[neighborhood_cell] + i]; neighbor.get() != std::addressof(object)) {
                    neighborhood->push_back(neighbor);
                }
            }
        }

        return neighborhood;
    }
}
