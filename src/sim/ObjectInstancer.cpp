//
// Created by moltmanns on 9/26/25.
//

#include "ObjectInstancer.h"

namespace swarmulator {
    void ObjectInstancer::update_gpu() {
        // all this does is update the ssbos!!
        for (auto& [id, group] : object_groups_) {
            const size_t group_size = group.objects.size(); // how much space do we need for the transfer?

            group.ssbo_buffer.resize(group_size); // resize the cpu buffer
            size_t i = 0;
            // and copy in the objects
            for (const auto obj : group.objects) {
                group.ssbo_buffer[i++] = obj->to_ssbo();
            }

            // check gpu buffer capacity and allocate new if necessary
            // allocation is just like std::vector - double capacity every time
            if (group_size > group.ssbo_capacity) {
                // delete old gpu buffer if present
                if (group.ssbo_id != 0) {
                    rlUnloadShaderBuffer(group.ssbo_id);
                }

                // find next power of 2 that fits for size
                group.ssbo_capacity = 1;
                while (group.ssbo_capacity < group_size) {
                    group.ssbo_capacity *= 2;
                }

                // init new buffers (also updates data)
                group.ssbo_buffer.resize(group.ssbo_capacity);
                group.ssbo_id = rlLoadShaderBuffer(group.ssbo_capacity * sizeof(SSBOObject), group.ssbo_buffer.data(), RL_DYNAMIC_COPY);
            }
            else {
                // just update buffer data
                rlUpdateShaderBuffer(group.ssbo_id, group.ssbo_buffer.data(), group.ssbo_capacity * sizeof(SSBOObject), 0);
            }
        }
    }

    void ObjectInstancer::draw(const object_group& group, const Matrix & projection, const Matrix & view) {
        const auto group_size = group.objects.size();

        // set shader info
        rlEnableShader(group.shader.id);
        // uniforms
        SetShaderValueMatrix(group.shader, group.shader_proj_mat_loc, projection);
        SetShaderValueMatrix(group.shader, group.shader_view_mat_loc, view);
        SetShaderValue(group.shader, group.shader_instance_count_loc, &group_size, SHADER_UNIFORM_INT);
        // bind the ssbo to the shader
        rlBindShaderBuffer(group.ssbo_id, 0);

        // draw
        rlEnableVertexArray(group.vao_id);
        rlDrawVertexArrayInstanced(0, 3, group_size);
        rlDisableVertexArray();
        rlDisableShader();
    }

    void ObjectInstancer::draw_all(const Matrix & view) const {
        const Matrix projection = rlGetMatrixProjection();
        for (const auto& [id, group] : object_groups_) {
            draw(group, projection, view);
        }
    }

    size_t ObjectInstancer::size() const {
        size_t size = 0;
        for (const auto& [id, group] : object_groups_) {
            size += group.objects.size();
        }
        return size;
    }

} // namespace swarmulator
