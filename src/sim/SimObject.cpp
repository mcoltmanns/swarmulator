//
// Created by moltmanns on 9/24/25.
//

#include <H5Cpp.h>

#include "SimObject.h"
#include "ObjectInstancer.h"

namespace swarmulator {
    SimObject::SSBOObject SimObject::to_ssbo() const {
        SSBOObject ssbo{};

        ssbo.position.x = position_.x;
        ssbo.position.y = position_.y;
        ssbo.position.z = position_.z;

        //ssbo.rotation.w = rotation_.w;
        ssbo.rotation.x = rotation_.x;
        ssbo.rotation.y = rotation_.y;
        ssbo.rotation.z = rotation_.z;

        ssbo.scale.x = scale_.x;
        ssbo.scale.y = scale_.y;
        ssbo.scale.z = scale_.z;

        return ssbo;
    }
} // namespace swarmulator
