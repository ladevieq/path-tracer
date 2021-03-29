#ifndef __SPHERE_HPP_
#define __SPHERE_HPP_

#include "aabb.hpp"
#include "material.hpp"

struct sphere {
    sphere() = default;
    sphere(point3 center, material material, float radius)
        : center(center), radius(radius), mat(material),
            bounding_box(center, radius)
    {}

    point3 center;

    material mat;

    aabb bounding_box;

    float radius;
    float padding[3];
};

#endif // !__SPHERE_HPP_
