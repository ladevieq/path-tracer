#include <iostream>

#include "color.hpp"
#include "vec3.hpp"
#include "ray.hpp"

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

bool hit_sphere(const point3& sphere_pos, double sphere_radius, const ray& r) {
    // Vector from the ray origin to sphere position
    vec3 origin_center = r.origin - sphere_pos;
    double a = r.direction.dot(r.direction);
    double b = 2.0 * r.direction.dot(origin_center);
    double c = origin_center.dot(origin_center) - sphere_radius * sphere_radius;
    double discriminant = b * b - 4.0 * a * c;

    return (discriminant > 0);
}

color ray_color(ray& r) {
    auto unit_dir = r.direction.unit();
    auto t = (unit_dir.y + 1.0) * 0.5;

    auto c = lerp(ground_color, sky_color, t);

    if (hit_sphere(point3{ 0.0, 0.0, -1.0 }, .5, r)) {
        c = color { 1.0, 0.0, 0.0 };
    }

    return c;
}

int main() {
    // Image dimensions
    const double aspect_ratio = 16.0 / 9.0;
    const size_t width = 400;
    const size_t height = width / aspect_ratio;
    // const size_t width = 256;
    // const size_t height = 256;

    // const double aspect_ratio = width / height;

    // Viewport dimensions
    const double viewport_height = 2.0;
    const double viewport_width = viewport_height * aspect_ratio;

    // Unit vectors for viewport pixels
    const vec3 horizontal { viewport_width / width, 0.0, 0.0 };
    const vec3 vertical { 0.0, viewport_height / height, 0.0 };

    // We use a right handed coord system
    const point3 eye_pos {};
    const double focal_length = 1.0;

    const point3 viewport_first_pixel {
        -viewport_width / 2.0,
        -viewport_height / 2.0,
        -focal_length
    };

    const point3 first_pixel = eye_pos + viewport_first_pixel;
    std::cerr << viewport_first_pixel << std::endl;

    // PPM format header
    std::cout << "P3\n" << width << '\n' << height << "\n 255" << std::endl;

    std::cerr << "Generating image" << std::endl;

    for (ssize_t row = height - 1; row >= 0; row--) {
        std::cerr << row << " lines remaining" << std::endl;
        for (size_t column = 0; column < width; column++) {
            vec3 x = horizontal * column;
            vec3 y = vertical * row;

            vec3 pixel_pos = first_pixel + x + y;
            vec3 direction = pixel_pos - eye_pos;

            ray ray { eye_pos, direction };

            write_color(std::cout, ray_color(ray));
            // write_color(std::cout, color(direction.unit().y, direction.unit().y, direction.unit().y));
            // write_color(std::cout, color(y.y, y.y, y.y));
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
