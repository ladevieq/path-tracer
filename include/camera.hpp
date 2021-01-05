#ifndef __CAMERA_HPP_
#define __CAMERA_HPP_

#include "ray.hpp"

class camera {
    public:
        camera(
            point3 position,
            point3 target,
            double v_fov,
            double aspect_ratio,
            double aperture,
            double focus_dist
        );

        ray get_ray(double u, double v) const;

        point3 position;
        float lens_radius;

        float viewport_width;
        float viewport_height;

        vec3 horizontal;
        vec3 vertical;

        vec3 forward;
        vec3 right;
        vec3 up;

        point3 bottom_left_pixel;
};

#endif // !__CAMERA_HPP_
