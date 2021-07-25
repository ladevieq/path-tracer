#ifndef __TRIANGLE_HPP_
#define __TRIANGLE_HPP_

#include "aabb.hpp"
#include "material.hpp"

struct vertex {
    vec3 position;
    vec3 normal;
};

struct triangle {
    triangle() = default;
    triangle(vertex v1, vertex v2, vertex v3)
        : v1(v1), v2(v2), v3(v3), bounding_box(v1.position, v2.position, v3.position)
    {}

    vertex v1;
    vertex v2;
    vertex v3;

    aabb bounding_box;
};

#endif // !__TRIANGLE_HPP_
