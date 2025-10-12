//
// Created by moltma on 10/12/25.
//

#ifndef SWARMULATOR_CPP_DTYPES_H
#define SWARMULATOR_CPP_DTYPES_H
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

#endif // SWARMULATOR_CPP_DTYPES_H
