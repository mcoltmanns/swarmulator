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
#include <vector>
#include <memory>
#include <map>

#include "SimObject.h"
#include "raylib.h"
#include "rlgl.h"

#define check_t_subtype_simobject static_assert(std::is_base_of_v<SimObject, T>, "Objects registered to or used with the instancer must be an instance of or derive from SimObject")

namespace swarmulator {
    class ObjectInstancer {
    public:
        struct object_group {
            std::list<SimObject*> objects{}; // all the objects part of this group (these must be pointers because of all the super/subclass stuff)
            Shader shader{}; // shader we use to draw these objects
            int shader_proj_mat_loc = 0;
            int shader_view_mat_loc = 0;
            int shader_instance_count_loc = 0;
            unsigned int vao_id = 0; // vao for the mesh used to draw these objects
            std::vector<SimObject::SSBOObject> ssbo_buffer; // instance information buffer array
            unsigned int ssbo_id = 0; // gpu id for the ssbo
            size_t ssbo_capacity = 0; // gpu-side ssbo capacity
        };

    private:
        // map type names
        std::map<size_t, object_group> object_groups_{};

        // simobject ids are unique for the lifetime of an objectinstancer
        size_t next_id_ = 0;

        template<class T>
        static size_t get_gid() { return typeid(T).hash_code(); }

        // draw a given group
        // wrap with calls to begin and end 3d mode
        static void draw(const object_group& group, const Matrix &projection, const Matrix &view);

    public:
        ObjectInstancer() = default;
        ~ObjectInstancer();

        // allocate a new object group from a type
        template<class T>
        void new_group(const std::vector<Vector3> &mesh, const std::string& vertex_src_path, const std::string& fragment_src_path) {
            check_t_subtype_simobject; // make sure t is a subtype of a simobject
            const auto gid = get_gid<T>();
            // make sure group isn't a duplicate
            if (object_groups_.contains(gid)) {
                throw std::runtime_error("Object group already exists.");
            }

            object_group group;

            // set up mesh
            group.vao_id = rlLoadVertexArray();
            rlEnableVertexArray(group.vao_id); // unloaded in destructor
            rlEnableVertexAttribute(0);
            rlLoadVertexBuffer(mesh.data(), static_cast<int>(sizeof(Vector3) * mesh.size()), false);
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, 0, 0);
            rlDisableVertexArray();

            // set up shader
            // load shader
            group.shader = LoadShader(vertex_src_path.c_str(), fragment_src_path.c_str()); // unloaded at destruction
            group.shader_proj_mat_loc = GetShaderLocation(group.shader, "proj_mat");
            if (group.shader_proj_mat_loc < 0) {
                throw std::runtime_error("Could not find shader projection matrix location");
            }
            group.shader_view_mat_loc = GetShaderLocation(group.shader, "view_mat");
            if (group.shader_view_mat_loc < 0) {
                throw std::runtime_error("Could not find shader view matrix location");
            }
            group.shader_instance_count_loc = GetShaderLocation(group.shader, "instance_count");
            if (group.shader_instance_count_loc < 0) {
                throw std::runtime_error("Could not find shader instance count location");
            }

            // set up ssbo buffer
            group.ssbo_capacity = 4096; // initial capacity - power of 2 please!
            group.ssbo_buffer.resize(group.ssbo_capacity);

            // set up ssbo
            group.ssbo_id = rlLoadShaderBuffer(group.ssbo_capacity * sizeof(SimObject::SSBOObject), group.ssbo_buffer.data(), RL_DYNAMIC_COPY); // unloaded in destructor

            object_groups_[gid] = group;
        }

        // add an object to its group
        // object id is set according to the next available object id
        // the object passed is copied, and management is taken over by the instancer
        // return a pointer to the copy managed by the instancer
        template<class T>
        SimObject* add_object(const T &obj) {
            check_t_subtype_simobject; // make sure t is a subtype of a simulation object (at compiletime)
            const auto gid = get_gid<T>();

            if (!object_groups_.contains(gid)) {
                throw std::runtime_error("Object group does not exist.");
            }

            object_group &group = object_groups_[gid];
            auto managed = new T(obj); // deleted in destructor
            group.objects.push_back(managed);
            group.objects.back()->set_id(next_id_++); // set id (doing it like this avoids having to cast)
            return group.objects.back();
        }

        // update shaders with group information
        void update_gpu();

        // remove an object from the simulation, given iterators to its group and itself within the group
        // using this manages the memory allocated by the instancer
        // yes, it could be static, but conceptually i like it not that way
        std::list<SimObject *>::iterator remove_object(std::map<size_t, object_group>::iterator group_it, std::list<SimObject *>::iterator object_it);

        // draw all groups
        // wrap with calls to begin and end 3d mode
        void draw_all(const Matrix &view) const;

        // iterator to the beginning of the object groups
        std::map<std::size_t, object_group>::iterator begin() { return object_groups_.begin(); }
        // iterator to the end of the object groups
        std::map<std::size_t, object_group>::iterator end() { return object_groups_.end(); }

        // reverse iterator to the beginning of the object groups
        std::map<std::size_t, object_group>::reverse_iterator rbegin() { return object_groups_.rbegin(); }
        // reverse iterator to the end of the object groups
        std::map<std::size_t, object_group>::reverse_iterator rend() { return object_groups_.rend(); }

        // number of objects currently in the instancer: sum of the size of all object lists in all groups
        [[nodiscard]] size_t size() const;

        // number of objects in a given object group
        template<class T>
        [[nodiscard]] size_t group_size() const {
            check_t_subtype_simobject;

            const auto gid = get_gid<T>();
            return object_groups_[gid].objects.size();
        }
    };
}

#endif //SWARMULATOR_CPP_OBJECTINSTANCER_H
