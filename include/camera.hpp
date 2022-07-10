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

        void set_aspect_ratio(float ratio);

        void move(const vec3& v);

        void rotate_y(float theta);


        point3 position;

        vec3 forward;
        vec3 right;
        vec3 up;

        vec3 horizontal;
        vec3 vertical;

        vec3 first_pixel;

        float lens_radius;
        float fov;
        float focus_distance;
        float aspect_ratio;
};

#endif // !__CAMERA_HPP_
