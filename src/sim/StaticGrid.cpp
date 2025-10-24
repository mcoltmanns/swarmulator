//
// Created by moltmanns on 9/24/25.
//

#include "StaticGrid.h"

namespace swarmulator {
    [[nodiscard]] int StaticGrid::cell_index(const Vector3 pos_grid) const {
        if (pos_grid.x < 0 || pos_grid.y < 0 || pos_grid.z < 0 || pos_grid.x >= world_size_.x || pos_grid.y >= world_size_.y || pos_grid.z >= world_size_.z) {
            return -1;
        }
        const auto [x, y, z] = floorv3(pos_grid / cell_size_);
        const int xpart = axis_cell_count_ * axis_cell_count_ * static_cast<int>(x);
        const int ypart = axis_cell_count_ * static_cast<int>(y);
        const int zpart = static_cast<int>(z);
        return xpart + ypart + zpart;
    }

    // get the 1d cell index of a given grid space position, and the indices of all cells surrounding that cell in a given radius
    // does not wrap around world boundaries
    [[nodiscard]] std::vector<int> StaticGrid::neighborhood_indices(const Vector3 &pos_grid, const float neighborhood_radius) const {
        std::vector<int> indices;
        // offsets are pos_grid - radius, pos_grid + radius
        for (int x = -neighborhood_radius; x <= neighborhood_radius; x += cell_size_.x) {
            for (int y = -neighborhood_radius; y <= neighborhood_radius; y += cell_size_.y) {
                for (int z = -neighborhood_radius; z <= neighborhood_radius; z += cell_size_.z) {
                    auto offset = Vector3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                    auto op = pos_grid + offset;
                    //op.x = wrap(op.x, world_size_.x);
                    //op.y = wrap(op.y, world_size_.y);
                    //op.z = wrap(op.z, world_size_.z);
                    if (auto index = cell_index(op); index != -1) {
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
        // nothing wrong in here. not sure why boids are attracted to the center!!
        sorted = std::vector<SimObject*>(in.size());
        segment_start = std::vector<uint32_t>(total_cell_count_, 0);
        segment_length = std::vector<uint32_t>(total_cell_count_, 0);

        // count the number of agents in each cell
        // slow with parallel
        for (auto grp = in.begin(); grp != in.end(); ++grp) {
            for (auto it = grp->second.objects.begin(); it != grp->second.objects.end(); ++it) {
                const auto obj_ptr = *it;
                const auto pos_grid = obj_ptr->get_position() + 0.5f * world_size_;
                if (const auto cell = cell_index(pos_grid); cell != -1) { // only add agents if they're in bounds
                    ++segment_start[cell];
                    ++segment_length[cell];
                }
            }
        }

        // compute prefix sum
        // not really worth the effort to parallelize
        for (int i = 1; i < total_cell_count_; i++) {
            segment_start[i] += segment_start[i - 1];
        }

        // sort agents into their cells
        // slow with parallel
        for (auto rgrp = in.rbegin(); rgrp != in.rend(); ++rgrp) { // careful! need to iterate in reverse here
            for (auto rit = rgrp->second.objects.rbegin(); rit != rgrp->second.objects.rend(); ++rit) {
                const auto obj_ptr = *rit;
                auto pos = obj_ptr->get_position();
                const auto pos_grid = pos + 0.5f * world_size_;
                if (const auto cell = cell_index(pos_grid); cell != -1) {
                    sorted[--segment_start[cell]] = obj_ptr;
                }
            }
        }
    }

    std::list<SimObject *> StaticGrid::get_neighborhood(const SimObject *object) const {
        auto neighborhood = std::list<SimObject*>();
        const auto object_pos = object->get_position();
        const auto object_pos_grid = object_pos + 0.5f * world_size_;

        // iterate over every cell in the neighborhood
        // this is not worth parallelizing - at worst 3^3=27 iterations
        for (const auto n_i = neighborhood_indices(object_pos_grid, object->get_interaction_radius()); const auto neighborhood_cell : n_i) {
            // iterate over every agent in the current neighborhood cell
            for (int i = 0; i < segment_length[neighborhood_cell]; i++) {
                // add the agent to the neighborhood vector if it isn't the agent we're getting the neighborhood of, and if it's within our interaction radius
                if (auto neighbor = sorted[segment_start[neighborhood_cell] + i];
                    neighbor != object &&
                    Vector3DistanceSqr(neighbor->get_position(), object->get_position()) <= object->get_interaction_radius() * object->get_interaction_radius()) {
                        neighborhood.push_back(neighbor);
                }
            }
        }

        return neighborhood;
    }
}
