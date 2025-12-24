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
    float v0 = forward.get<0>();
    float v2 = forward.get<2>();
    float new_v0 = v0 * cos(theta) - v2 * sin(theta);
    float new_v2 = v0 * sin(theta) + v2 * cos(theta);
    forward.set<0>(new_v0);
    forward.set<2>(new_v2);
    forward.normalize();

    right = forward.cross(vec3{ 0.0, 1.0, 0.0 }).normalize();
    up = right.cross(forward).normalize();

    auto h = tan(deg_to_rad(fov) / 2.f);
    auto viewport_height = 2.f * h;
    auto viewport_width = viewport_height * aspect_ratio;

    horizontal  = focus_distance * viewport_width * right;
    vertical    = focus_distance * viewport_height * up;

    vertical    = focus_distance * viewport_height * up;
    first_pixel = position - horizontal / 2.0 - vertical / 2.0 + focus_distance * forward;
}
