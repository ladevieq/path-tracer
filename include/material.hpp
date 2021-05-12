#ifndef __MATERIAL_HPP_
#define __MATERIAL_HPP_

#include "vec3.hpp"

enum MATERIAL_TYPE: uint32_t {
    LAMBERTIAN  = 0,
    METAL       = 1,
    DIELECTRIC  = 2,
    EMISSIVE    = 3,
};

struct material {
    color albedo;
    color emissive;
    float fuzz;
    float ior;
    uint32_t type;
    uint32_t padding;
};

#endif // !__MATERIAL_HPP_
