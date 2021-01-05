#include "camera.hpp"

#include "rt.hpp"

camera::camera(
        point3 position,
        point3 target,
        double v_fov,
        double aspect_ratio,
        double aperture,
        double focus_dist
    ) : position(position), lens_radius(aperture / 2.0) {

    forward = (target - position).unit();
    right = forward.cross(vec3{ 0.0, 1.0, 0.0 }).unit();
    up = right.cross(forward).unit();

    auto h = tan(deg_to_rad(v_fov) / 2.0);
    viewport_height = 2.0 * h;
    viewport_width = viewport_height * aspect_ratio;

    horizontal  = focus_dist * viewport_width * right;
    vertical    = focus_dist * viewport_height * up;

    bottom_left_pixel = position - horizontal / 2.0 - vertical / 2.0 - forward * focus_dist;
}

ray camera::get_ray(double u, double v) const {
    vec3 rd = lens_radius * random_in_unit_disk();
    vec3 offset = rd.x * right + rd.y * up;

    point3 viewport_pixel = bottom_left_pixel + u * horizontal + v * vertical;

    return ray {
        position + offset,
        viewport_pixel - position - offset
    };
}
