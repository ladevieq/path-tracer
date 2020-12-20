#include "ray.hpp"

ray::ray(const point3& origin, const vec3& direction): origin(origin), direction(direction)
{}

point3 ray::at(double t) {
    return origin + direction * t;
}
