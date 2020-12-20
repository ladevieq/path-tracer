#include "camera.hpp"

camera::camera(point3 position, double aspect_ratio, double focal_length)
    : position(position), focal_length(focal_length) {
    auto viewport_height = 2.0;
    auto viewport_width =  viewport_height * aspect_ratio;

    horizontal = vec3 { viewport_width, 0.0, 0.0 };
    vertical = vec3 { 0.0, viewport_height, 0.0 };

    bottom_left_pixel = position - horizontal / 2.0 - vertical / 2.0 - vec3 { 0.0, 0.0, focal_length };

    std::cerr << bottom_left_pixel << std::endl;
}

ray camera::get_ray(double u, double v) const {
    point3 viewport_pixel = bottom_left_pixel + u * horizontal + v * vertical;
    return ray {
        position,
        viewport_pixel - position
    };
}
