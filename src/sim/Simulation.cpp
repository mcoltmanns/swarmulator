//
// Created by moltmanns on 9/24/25.
//

#include "Simulation.h"

namespace swarmulator {
    void Simulation::update(const float dt) {
        total_time_ += dt;
        ++total_steps_;

        // remove inactive objects, wrap bounds
        for (auto group_it = object_instancer_.begin(); group_it != object_instancer_.end(); ++group_it) {
            auto obj_it = group_it->second.objects.begin();
            while (obj_it != group_it->second.objects.end()) {
                if (const auto obj_ptr = *obj_it; !obj_ptr->active()) {
                    obj_it = object_instancer_.remove_object(group_it, obj_it);
                }
                else {
                    obj_ptr->set_position(wrap_position(obj_ptr->get_position(), world_size_));
                    ++obj_it;
                }
            }
        }

        // sort everything (don't seem to be issues here)
        grid_.sort_objects(object_instancer_);
        // begin a logging frame
        logger_.queue_begin_frame(total_time_);

        // update everyone
        // iteration is cheap, processing is expensive
        // so have every thread iterate over the whole list
        // but only a single thread will access a specific element (single) while the others continue on (nowait)
        // this has no noticeable performance impact vs parallel vector access
        // worth parallelizing the outer loop? not unless number of object groups is large.
        for (auto grp = object_instancer_.begin(); grp != object_instancer_.end(); ++grp) {
            // update objects
            std::list<SimObject*>::iterator it;
#pragma omp parallel private(it) shared(grp, dt) default(none)
            {
                for (it = grp->second.objects.begin(); it != grp->second.objects.end(); ++it) {
#pragma omp single nowait
                    {
                        const auto object = *it;
                        auto neighborhood = grid_.get_neighborhood(object);
                        object->update(neighborhood, dt);
                        logger_.queue_log_object_data(object->type_name(), object->log(), true);
                    }
                }
            }
        }

        // update gpu
        object_instancer_.update_gpu();

        // end a logging frame
        // next logging frame
        logger_.queue_advance_frame();
    }

    void Simulation::draw_objects(const Matrix & view) const {
        object_instancer_.draw_all(view);
    }
} // swarmulator