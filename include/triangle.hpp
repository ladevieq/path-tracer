#ifndef __TRIANGLE_HPP_
#define __TRIANGLE_HPP_

struct vertex {
    vec3 position;
    vec3 normal;
};

struct triangle {
    triangle() = default;
    triangle(vertex v1, vertex v2, vertex v3)
        : v1(v1), v2(v2), v3(v3)
    {}

    vec3 center() {
        return (v1.position + v2.position + v3.position) / 3.f;
    }

    vertex v1;
    vertex v2;
    vertex v3;
};

#endif // !__TRIANGLE_HPP_
