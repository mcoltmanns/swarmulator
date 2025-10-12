//
// Created by moltmanns on 9/26/25.
//

#include "ObjectInstancer.h"

#include "rlgl.h"

#define check_t_subtype_simobject static_assert(std::is_base_of_v<SimObject, T>, "Objects registered to or used with the instancer must be or derive from SimObject")

ObjectInstancer::ObjectInstancer() = default;

ObjectInstancer::~ObjectInstancer() {
    // free all the raylib stuff
    auto it = object_groups_.begin();
    while (it != object_groups_.end()) {
        it = free_object_type_intern(it);
    }
}

std::map<size_t, object_type_group>::iterator ObjectInstancer::free_object_type_intern(
    const std::map<size_t, object_type_group>::iterator &pair) {
    const auto group = std::move(pair->second); // we are freeing an object type group, so ok to take control of it

    // unload the shaders
    UnloadShader(group.shader);

    // vao need not be unloaded
    // free ssbo
    rlUnloadShaderBuffer(group.ssbo);
    // free ssbo buffer
    RL_FREE(group.ssbo_buffer);

    // don't need to unload the object list elements because they're unique pointers
    // just erase the type group (and by assoc, the object list)
    status_ = OK;
    return object_groups_.erase(pair);
}


template<class T>
size_t ObjectInstancer::calloc_object_type(std::string& vtx_src_p, std::string& frg_src_p, std::vector<Vector3>& mesh) {
    check_t_subtype_simobject;

    const auto key = typeid(T).hash_code();
    if (object_groups_.contains(key)) {
        status_ = BAD_ALLOC; // if we failed, set the flag
        return key;
    }

    // load the shaders
    const auto shader = LoadShader(vtx_src_p.c_str(), frg_src_p.c_str());

    // allocate the vao
    const auto vao = rlLoadVertexArray();
    // and load it
    rlEnableVertexArray(vao);
    rlEnableVertexAttribute(0); // TODO is 0 always ok here?
    rlLoadVertexBuffer(mesh.data(), sizeof(mesh), false);
    rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
    rlDisableVertexArray();

    // allocate the ssbo buffer
    const auto ssbo_buffer = static_cast<SSBOObject*>(RL_CALLOC(max_group_size_, sizeof(SSBOObject)));
    // and the ssbo
    const auto ssbo = rlLoadShaderBuffer(max_group_size_ * sizeof(SSBOObject), ssbo_buffer, RL_DYNAMIC_COPY);

    rlCheckErrors();

    // then throw everything into the map under a new object type group
    object_groups_.emplace(key,
        object_type_group {{},
            shader,
            vao,
            ssbo,
            ssbo_buffer,
            {} // empty object pointer list since this is a new group
        }
    );

    // and then finally give the user the key to the thing they alloc'd
    status_ = OK;
    return key;
}

void ObjectInstancer::free_object_type(const size_t key) {
    // check if the thing was allocated
    const auto pair = object_groups_.find(key);
    if (pair == object_groups_.end()) {
        status_ = BAD_ALLOC;
        return;
    }
    free_object_type_intern(pair);
}

