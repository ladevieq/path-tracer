#ifndef __CAMERA_HPP_
#define __CAMERA_HPP_

#include "vec3.hpp"

class camera {
    public:
        camera(
            point3 position,
            point3 target,
            float v_fov,
            float aspect_ratio,
            float aperture,
            float focus_dist
        );

        point3 position;

        vec3 right;
        vec3 up;

        vec3 horizontal;
        vec3 vertical;

        vec3 first_pixel;

        float lens_radius;
        float padding[3];
};

#endif // !__CAMERA_HPP_
