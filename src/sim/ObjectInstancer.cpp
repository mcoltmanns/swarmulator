//
// Created by moltmanns on 9/26/25.
//

#include "ObjectInstancer.h"

#include "rlgl.h"

namespace swarmulator {
#define check_t_subtype_simobject static_assert(std::is_base_of_v<SimObject, T>, "Objects registered to or used with the instancer must be or derive from SimObject")

    ObjectInstancer::ObjectInstancer() = default;

    ObjectInstancer::~ObjectInstancer() {
        // TODO write this method!
    }

    void ObjectInstancer::free_object_group(const size_t key) {
        // check if the thing was allocated
        const auto info = object_groups_.find(key);
        if (info == object_groups_.end()) {
            throw std::runtime_error("Cannot free non-existent object type.");
        }

        // shared pointers, so just erase the object group
        info->second.objects.clear();

        // free object info
        rlUnloadShaderProgram(info->second.shader.id);
        rlUnloadVertexArray(info->second.vao);
        rlUnloadShaderBuffer(info->second.ssbo);
        RL_FREE(info->second.ssbo_buffer);
        object_groups_.erase(info);
    }

    size_t ObjectInstancer::size() const {
        // calculate the total number of objects in the instancer
        // o(number of object groups)
        size_t total = 0;
        for (const auto& g : object_groups_) {
            total += g.second.objects.size();
        }
        return total;
    }
} // namespace swarmulator
