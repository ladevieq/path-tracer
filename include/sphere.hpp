#ifndef __SPHERE_HPP_
#define __SPHERE_HPP_

#include "material.hpp"

struct sphere{
    sphere() = default;
    sphere(point3 center, material material, float radius);

    point3 center;

    material mat;

    float radius;
    float padding[3];
};

#endif // !__SPHERE_HPP_
