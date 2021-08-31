#ifndef __MATERIAL_HPP_
#define __MATERIAL_HPP_

#include "vec3.hpp"

struct material {
    color base_color;
    float metalness = 0.f;
    float roughness = 1.f;
    uint32_t padding[2];
};

#endif // !__MATERIAL_HPP_
