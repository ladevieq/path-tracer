#include "camera.hpp"

#include "rt.hpp"

camera::camera(point3 position, point3 target, double v_fov, double aspect_ratio)
    : position(position) {

    auto forward = (position - target).unit();
    auto right = vec3{ 0.0, 1.0, 0.0 }.cross(forward).unit();
    auto up = forward.cross(right).unit();

    auto h = tan(deg_to_rad(v_fov) / 2.0);
    auto viewport_height = 2.0 * h;
    auto viewport_width = viewport_height * aspect_ratio;

    horizontal = viewport_width * right;
    vertical = viewport_height * up;

    bottom_left_pixel = position - horizontal / 2.0 - vertical / 2.0 - forward;
}

ray camera::get_ray(double u, double v) const {
    point3 viewport_pixel = bottom_left_pixel + u * horizontal + v * vertical;
    return ray {
        position,
        viewport_pixel - position
    };
}
