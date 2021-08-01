#ifndef __AABB_HPP_
#define __AABB_HPP_

#include "vec3.hpp"
#include "triangle.hpp"
#include <cmath>
#include <limits.h>

class aabb {
    public:
        aabb() = default;
        aabb(vec3 &min_point, vec3 &max_point): minimum(min_point), maximum(max_point) {}
        aabb(vec3 &position, float radius): minimum(position - vec3(radius)), maximum(position + vec3(radius)) {}
        aabb(triangle &tri) {
            auto &v1 = tri.v1.position;
            auto &v2 = tri.v2.position;
            auto &v3 = tri.v3.position;

            minimum.x = fmin(fmin(v1.x, v2.x), v3.x);
            minimum.y = fmin(fmin(v1.y, v2.y), v3.y);
            minimum.z = fmin(fmin(v1.z, v2.z), v3.z);

            maximum.x = fmax(fmax(v1.x, v2.x), v3.x);
            maximum.y = fmax(fmax(v1.y, v2.y), v3.y);
            maximum.z = fmax(fmax(v1.z, v2.z), v3.z);
        }

        void union_with(const aabb &box) {
            minimum.x = fmin(minimum.x, box.minimum.x);
            minimum.y = fmin(minimum.y, box.minimum.y);
            minimum.z = fmin(minimum.z, box.minimum.z);

            maximum.x = fmax(maximum.x, box.maximum.x);
            maximum.y = fmax(maximum.y, box.maximum.y);
            maximum.z = fmax(maximum.z, box.maximum.z);
        }

        void union_with(const point3 &point) {
            minimum.x = fmin(minimum.x, point.x);
            minimum.y = fmin(minimum.y, point.y);
            minimum.z = fmin(minimum.z, point.z);

            maximum.x = fmax(maximum.x, point.x);
            maximum.y = fmax(maximum.y, point.y);
            maximum.z = fmax(maximum.z, point.z);
        }

        int32_t maximum_axis() {
            vec3 diag = diagonal();

            if (diag.x > diag.y && diag.x > diag.z) {
                return 0;
            } else if (diag.y > diag.z) {
                return 1;
            } else {
                return 2;
            }
        }

        vec3 diagonal() {
            return maximum - minimum;
        }

        vec3 center() {
            return diagonal() / 2.f;
        }

        float surface_area() {
            auto diag = diagonal();
            return 2.f * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
        }

        vec3 minimum = vec3(std::numeric_limits<float>::max());
        vec3 maximum = vec3(std::numeric_limits<float>::lowest());
};

#endif // !__AABB_HPP_
