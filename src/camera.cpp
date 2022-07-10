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
    ) : position(position), lens_radius(aperture / 2.f), fov(v_fov), focus_distance(focus_dist) {

    forward = (target - position).normalize();
    right = forward.cross(vec3{ 0.0, 1.0, 0.0 }).normalize();
    up = right.cross(forward).normalize();

    set_aspect_ratio(aspect_ratio);
}

void camera::set_aspect_ratio(float ratio) {
    aspect_ratio = ratio;

    auto h = tan(deg_to_rad(fov) / 2.f);
    auto viewport_height = 2.f * h;
    auto viewport_width = viewport_height * aspect_ratio;

    horizontal  = focus_distance * viewport_width * right;
    vertical    = focus_distance * viewport_height * up;

    first_pixel = position - horizontal / 2.0 - vertical / 2.0 + focus_distance * forward;
}

void camera::move(const vec3& v) {
    position += v;
    first_pixel = position - horizontal / 2.0 - vertical / 2.0 + focus_distance * forward;
}

void camera::rotate_y(float theta) {
    forward.v[0] = forward.v[0] * cos(theta) - forward.v[2] * sin(theta);
    forward.v[2] = forward.v[0] * sin(theta) + forward.v[2] * cos(theta);
    forward.normalize();

    right = forward.cross(vec3{ 0.0, 1.0, 0.0 }).normalize();
    up = right.cross(forward).normalize();

    auto h = tan(deg_to_rad(fov) / 2.f);
    auto viewport_height = 2.f * h;

    vertical    = focus_distance * viewport_height * up;
    first_pixel = position - horizontal / 2.0 - vertical / 2.0 + focus_distance * forward;
}
