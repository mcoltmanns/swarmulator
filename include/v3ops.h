//
// Created by moltmanns on 4/7/25.
//

#ifndef V3OPS_H
#define V3OPS_H
#include <cmath>


#include "raylib.h"

// couple vector3 operators that raymath doesn't provide
inline Vector3 operator*(const float s, const Vector3 &r) { return Vector3(r.x * s, r.y * s, r.z * s); }
inline Vector3 floorv3(const Vector3 &v) { return Vector3(floor(v.x), floorf(v.y), floorf(v.z)); }
inline Vector3 xyz(const Vector4 &in) { return Vector3(in.x, in.y, in.z); }

#endif //V3OPS_H
