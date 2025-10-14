//
// Created by moltmanns on 9/24/25.
//

#include "Simulation.h"

namespace swarmulator {
    void Simulation::update(const float dt) {
        total_time_ += dt;

        // remove inactive objects, wrap bounds, reset buffer write places
        for (auto grp = object_instancer_.groups_begin(); grp != object_instancer_.groups_end(); ++grp) {
            grp->second.ssbo_buffer_write_place = 0;
            auto it = grp->second.objects.begin();
            while (it != grp->second.objects.end()) {
                if (!(*it)->active()) {
                    it = grp->second.objects.erase(it);
                }
                else {
                    (*it)->set_position(grid_.wrap_position((*it)->get_position()));
                    ++it;
                }
            }
        }

        // sort everything
        grid_.sort_objects(object_instancer_);

        // update everyone
        // iteration is cheap, processing is expensive
        // so have every thread iterate over the whole list
        // but only a single thread will access a specific element (single) while the others continue on (nowait)
        // this has no noticeable performance impact vs parallel vector access
        std::map<size_t, object_group>::iterator grp;
        std::list<SimObject*>::iterator it;
    #pragma omp parallel private(grp, it)
        {
            for (grp = object_instancer_.groups_begin(); grp != object_instancer_.groups_end(); ++grp) {
                // update objects
                for (it = grp->second.objects.begin(); it != grp->second.objects.end(); ++it) {
#pragma omp single nowait
                    {
                        const auto object = *it; // raw pointer to the object!
                        auto neighborhood = grid_.get_neighborhood(*object, object->get_interaction_radius());// passes the object by reference
                        const auto new_object = object->update(*neighborhood, dt);
                        // this critical section is actually pretty fast
        #pragma omp critical
                        {
                            object->write_to_ssbo(grp->second.ssbo_buffer[grp->second.ssbo_buffer_write_place]);
                            ++grp->second.ssbo_buffer_write_place;
                            if (new_object != nullptr) { // TODO add back check if can add object before you go doing it
                                add_object(new_object);
                                const auto p = Vector4{(randfloat() - 0.5f) * world_size_.x, (randfloat() - 0.5f) * world_size_.y, (randfloat() - 0.5f) * world_size_.z, 0};
                                const auto r = Vector4{randfloat() - 0.5f, randfloat() - 0.5f, randfloat() - 0.5f, 0};
                                new_object->set_position(xyz(p));
                                new_object->set_rotation(xyz(r));
                                new_object->write_to_ssbo(grp->second.ssbo_buffer[grp->second.ssbo_buffer_write_place]);
                                ++grp->second.ssbo_buffer_write_place;
                            }
                        }
                    }
                }
            }
        }

        // go thru all the object groups and copy their ssbo data
        // probably good to do this in parallel too? what would the bottleneck be here?
        // is updateShaderBuffer threadsafe?
        for (grp = object_instancer_.groups_begin(); grp != object_instancer_.groups_end(); ++grp) {
            rlUpdateShaderBuffer(grp->second.ssbo, grp->second.ssbo_buffer, grp->second.objects.size() * sizeof(SSBOObject), 0);
            //SSBOObject buffer_out[grp->second.objects.size()];
        }
    }

    void Simulation::draw_objects(const Matrix & projection, const Matrix & view) {
        // is this worth parallelizing?
        for (auto gpair = object_instancer_.groups_begin(); gpair != object_instancer_.groups_end(); ++gpair) {
            const auto & grp = object_instancer_.groups_begin()->second;
            // set values in object group vertex shader
            rlEnableShader(grp.shader.id);
            SetShaderValueMatrix(grp.shader, 0, projection);
            SetShaderValueMatrix(grp.shader, 1, view);
            const auto num_objects = grp.objects.size();
            SetShaderValue(grp.shader, 2, &num_objects, SHADER_UNIFORM_INT);
            // bind the object buffer
            rlBindShaderBuffer(grp.ssbo, 0);
            // draw instanced
            rlEnableVertexArray(grp.vao);
            rlDrawVertexArrayInstanced(0, grp.mesh.vertexCount, static_cast<int>(num_objects));
            // clean up
            rlDisableVertexArray();
            rlDisableShader();
            rlCheckErrors();
        }
    }
} // swarmulator