//
// Created by moltmanns on 9/26/25.
//
/*
 * the object instancer manages the loading, binding, and unloading of the
 * various buffers, meshes, and shaders that are required to make instanced draw
 * calls.
 */

#ifndef SWARMULATOR_CPP_OBJECTINSTANCER_H
#define SWARMULATOR_CPP_OBJECTINSTANCER_H
#include <list>
#include <memory>
#include <map>
#include <fstream>

#include "SimObject.h"
#include "rlgl.h"

#define check_t_subtype_simobject static_assert(std::is_base_of_v<SimObject, T>, "Objects registered to or used with the instancer must be or derive from SimObject")

namespace swarmulator {
    struct object_group {
        Shader shader{}; // the shader that all these objects are drawn with
        Mesh mesh{};
        unsigned int vao{}; // the vertices that all these objects are drawn with
        unsigned int ssbo{}; // the ssbos that all these objects read their info into
        SSBOObject* ssbo_buffer{}; // the buffer into which object info is written before the copy to the gpu
        unsigned int ssbo_buffer_write_place = 0;
        std::list<SimObject*> objects{};
    };

    class ObjectInstancer {
    private:
        static constexpr size_t max_objects_ = 10000; // maximum number of objects allowed
        static constexpr size_t max_group_size_ = 5000; // maximum number of objects allowed in a group (used to allocate gpu buffers)

        // map type hash codes to pointers to lists of simobjects and their associated info
        std::map<std::size_t, object_group> object_groups_ {};

    public:
        ObjectInstancer();
        ~ObjectInstancer();

        // load and compile shader sources, initialize a mesh vao, allocate an ssbo for objects of a given type
        // if the object type was already allocated, sets the instancer error flag and does nothing
        // you must initialize the raylib context before this is called!
        // returns a key corresponding to the hash code of the object type alloc'd
        template<class T>
        size_t alloc_object_group(const std::string &vtx_src_p, const std::string &frg_src_p, const Mesh &mesh) {
            check_t_subtype_simobject;

            const auto key = typeid(T).hash_code();
            if (object_groups_.contains(key)) {
                throw std::runtime_error("Object group already exists.");
            }

            // load the shaders
            const auto shader = LoadShader(vtx_src_p.c_str(), frg_src_p.c_str());

            // allocate the vao
            const auto vao = rlLoadVertexArray();
            // and load it
            rlEnableVertexArray(vao);
            rlEnableVertexAttribute(0); // TODO is 0 always ok here?
            rlLoadVertexBuffer(mesh.vertices, mesh.vertexCount, false);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
            rlDisableVertexArray();

            // allocate the ssbo buffer
            const auto ssbo_buffer = static_cast<SSBOObject*>(RL_CALLOC(max_group_size_, sizeof(SSBOObject)));
            // and the ssbo
            const auto ssbo = rlLoadShaderBuffer(max_group_size_ * sizeof(SSBOObject), ssbo_buffer, RL_DYNAMIC_COPY);

            rlCheckErrors();

            // then throw everything into the maps under a new object type group
            object_groups_.emplace(key,
                object_group {
                    shader,
                    mesh,
                    vao,
                    ssbo,
                    ssbo_buffer,
                    {},
                }
            );

            // and then finally give the user the key to the thing they alloc'd
            return key;
        }

        template<class T>
        void free_object_group() {
            check_t_subtype_simobject;

            const auto key = typeid(T).hash_code();
            free_object_group(key);
        }

        template<class T>
        object_group& get_object_group() {
            check_t_subtype_simobject;

            const auto key = typeid(T).hash_code();
            const auto it = object_groups_.find(key);
            if (it == object_groups_.end()) {
                throw std::runtime_error("Object group does not exist.");
            }
            return it->second;
        }

        template<class T>
        void add_object(T *object) {
            check_t_subtype_simobject;

            if (size() + 1 > max_objects_) {
                throw std::runtime_error("Maximum objects exceeded.");
            }

            const auto key = typeid(T).hash_code();
            const auto place = object_groups_.find(key);
            if (place == object_groups_.end()) {
                throw std::runtime_error("Object group does not exist.");
            }
            if (place->second.objects.size() + 1 > max_group_size_) {
                throw std::runtime_error("Maximum objects in group exceeded.");
            }
            place->second.objects.push_back(object);
        }

        // frees all the resources associated with a given object type hash code
        void free_object_group(size_t key);

        object_group& get_object_group(const size_t key) { return object_groups_.at(key); }

        size_t size() const;

        std::map<size_t, object_group>::iterator groups_begin() { return object_groups_.begin(); }
        std::map<size_t, object_group>::iterator groups_end() { return object_groups_.end(); }

        std::map<size_t, object_group>::reverse_iterator groups_rbegin() { return object_groups_.rbegin(); }
        std::map<size_t, object_group>::reverse_iterator groups_rend() { return object_groups_.rend(); }
    };
}

#endif //SWARMULATOR_CPP_OBJECTINSTANCER_H
