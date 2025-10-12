//
// Created by moltmanns on 9/24/25.
//

#include "Simulation.h"

namespace swarmulator {
void Simulation::add_object(std::unique_ptr<SimObject> object) {
    objects_.emplace_back(std::move(object));
}

void Simulation::update(const float dt) {
    total_time_ += dt;

    // remove inactive objects, wrap bounds
    auto it = objects_.begin();
    while (it != objects_.end()) {
        if (!(*it)->active()) {
            it = objects_.erase(it);
        }
        else {
            (*it)->set_position(grid_.wrap_position((*it)->get_position()));
            ++it;
        }
    }

    // sort everything
    grid_.sort_objects(objects_);

    // update everyone
    // iteration is cheap, processing is expensive
    // so have every thread iterate over the whole list
    // but only a single thread will access a specific element (single) while the others continue on (nowait)
    // this has no noticeable performance impact vs parallel vector access
    // but this is still the most expensive step (about 2x slower than grid sorting)
    // we could try to move to the gpu, but then we'd need to either write the agent processing code in glsl
    // check openmp target directive - may be possible to offload this directly to gpu
    int buffer_write_place = 0;
#pragma omp parallel private(it)
    {
        // update objects
        for (it = objects_.begin(); it != objects_.end(); ++it) {
#pragma omp single nowait
            {
                const auto object = it->get(); // raw pointer to the object!
                auto neighborhood = grid_.get_neighborhood(*object, object->get_interaction_radius());// passes the object by reference
                auto new_object = object->update(*neighborhood, dt);
                // this critical section is actually pretty fast
#pragma omp critical
                {
                    object->write_to_ssbo(objects_ssbo_array_[buffer_write_place]);
                    ++buffer_write_place;
                    if (new_object != nullptr && can_add_object()) {
                        add_object(std::move(new_object)); // hand over ownership of the new object to the simulation!
                        const auto p = Vector4{(randfloat() - 0.5f) * world_size_.x, (randfloat() - 0.5f) * world_size_.y, (randfloat() - 0.5f) * world_size_.z, 0};
                        const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
                        new_object->set_position(xyz(p));
                        new_object->set_rotation(xyz(r));
                        new_object->write_to_ssbo(objects_ssbo_array_[buffer_write_place]);
                        ++buffer_write_place;
                    }
                }
            }
        }
    }
    rlUpdateShaderBuffer(objects_ssbo_id_, objects_ssbo_array_, objects_.size() * sizeof(SSBOObject), 0); // only copy as much data as there are agents (actually saves loads of time)
}
} // swarmulator