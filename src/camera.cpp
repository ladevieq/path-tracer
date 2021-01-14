#include <cmath>

#include "camera.hpp"
#include "utils.hpp"

camera::camera(
        point3 position,
        point3 target,
        float v_fov,
        float aspect_ratio,
        float aperture,
        float focus_dist
    ) : position(position), lens_radius(aperture / 2.0) {

    auto forward = (target - position).unit();
    right = forward.cross(vec3{ 0.0, 1.0, 0.0 }).unit();
    up = right.cross(forward).unit();

    auto h = tan(deg_to_rad(v_fov) / 2.0);
    auto viewport_height = 2.0 * h;
    auto viewport_width = viewport_height * aspect_ratio;

    horizontal  = focus_dist * viewport_width * right;
    vertical    = focus_dist * viewport_height * up;

    first_pixel = position - horizontal / 2.0 - vertical / 2.0 + focus_dist * forward;
}
