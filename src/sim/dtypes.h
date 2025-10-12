//
// Created by moltma on 10/12/25.
//

#ifndef SWARMULATOR_CPP_DTYPES_H
#define SWARMULATOR_CPP_DTYPES_H
#include <list>


#include "raylib.h"

enum ObjectInstancerStatus {
    OK,
    BAD_ALLOC,
    BAD_FREE,
};

struct SSBOObject {
    Vector4 position;
    Vector4 rotation;
    Vector4 scale;
    Vector4 info;
};

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

#endif // SWARMULATOR_CPP_DTYPES_H
