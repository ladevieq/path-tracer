#ifndef __CAMERA_HPP_
#define __CAMERA_HPP_

#include "ray.hpp"

class camera {
    public:
        camera(point3 position, point3 target, double v_fov, double aspect_ratio);

        ray get_ray(double u, double v) const;

        point3 position;

        vec3 horizontal;
        vec3 vertical;

        point3 bottom_left_pixel;
};

#endif // !__CAMERA_HPP_
