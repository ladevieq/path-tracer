#ifndef __TRIANGLE_HPP_
#define __TRIANGLE_HPP_

#include "vec3.hpp"

struct triangle {
    triangle() = default;
    triangle(vec3 p1, vec3 p2, vec3 p3)
        : p1(p1), p2(p2), p3(p3)
    {
        center = (p1 + p2 + p3) / 3.f;
    }

    vec3 p1;
    vec3 p2;
    vec3 p3;
    vec3 center;
};

#endif // !__TRIANGLE_HPP_
