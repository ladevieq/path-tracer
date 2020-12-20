#ifndef __CAMERA_HPP_
#define __CAMERA_HPP_

#include "rt.hpp"

class camera {
    public:
        camera(point3 position, double aspect_ratio, double focal_length);

        ray get_ray(double u, double v) const;

        point3 position;
        double focal_length;

        vec3 horizontal;
        vec3 vertical;

        point3 bottom_left_pixel;
};

#endif // !__CAMERA_HPP_
