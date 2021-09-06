#ifndef __TRIANGLE_HPP_
#define __TRIANGLE_HPP_

#include "material.hpp"

struct vertex {
    vec3 position;
    vec3 normal;
    vec3 uv;
};

struct triangle {
    triangle() = default;
    triangle(vertex v1, vertex v2, vertex v3, material mat)
        : v1(v1), v2(v2), v3(v3), mat(mat)
    {}

    vec3 center() {
        return (v1.position + v2.position + v3.position) / 3.f;
    }

    vertex v1;
    vertex v2;
    vertex v3;
    material mat;
};

#endif // !__TRIANGLE_HPP_
