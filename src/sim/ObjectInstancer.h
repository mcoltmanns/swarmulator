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

struct non_copyable {
    non_copyable() = default;
    // this is a struct which is non-copyable (only std::moveable)
    // delete copy constructor
    non_copyable(const non_copyable&) = delete;
    non_copyable& operator=(const non_copyable&) = delete;

    // enable move
    non_copyable(non_copyable&&) = default;
    non_copyable& operator=(non_copyable&&) = default;
};

struct object_type_group : non_copyable {
    Shader shader; // the shader that all these objects are drawn with
    unsigned int vao; // the vertices that all these objects are drawn with
    unsigned int ssbo; // the ssbos that all these objects read their info into
    SSBOObject* ssbo_buffer; // the buffer into which object info is written before the copy to the gpu
    std::list<std::unique_ptr<SimObject>> object_list; // the list of all objects in this group currently in the simulation (instancer owns objects, so unique_ptr here is good)
};

class ObjectInstancer {
private:
    static constexpr size_t max_group_size_ = 2000; // maximum number of objects in a group

    // map type hash codes to pointers to lists of unique ptrs to simobjects
    std::map<std::size_t, object_type_group> object_groups_ {};

    ObjectInstancerStatus status_ = ObjectInstancerStatus::OK;

    // returns the iterator after erase
    std::map<size_t, object_type_group>::iterator free_object_type_intern(
        const std::map<size_t, object_type_group>::iterator &pair);

public:
    ObjectInstancer();
    ~ObjectInstancer();

    // certain methods, when they fail, will set the objectinstancer's status flag to a value other than OK
    // status reports that value
    [[nodiscard]] ObjectInstancerStatus status() const { return status_; }
    // reset instancer status flag (probably you should call this after you check status)
    void reset_status() { status_ = ObjectInstancerStatus::OK; }

    // load and compile shader sources, initialize a mesh vao, allocate an ssbo for objects of a given type
    // if the object type was already allocated, sets the instancer error flag and does nothing
    // you must initialize the raylib context before this is called!
    // returns a key corresponding to the hash code of the object type alloc'd
    template<class T>
    size_t calloc_object_type(std::string& vtx_src_p, std::string& frg_src_p, std::vector<Vector3>& mesh);

    // frees all the resources associated with a given object type hash code
    void free_object_type(size_t key);
};


#endif //SWARMULATOR_CPP_OBJECTINSTANCER_H
