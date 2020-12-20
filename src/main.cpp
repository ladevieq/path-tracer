#include <iostream>

#include "color.hpp"
#include "vec3.hpp"
#include "ray.hpp"

color ray_color(ray& r) {
    auto unit_dir = r.direction.unit();
    auto t = (unit_dir.y + 1.0) * 0.5;

    return lerp(color(1.0, 1.0, 1.0), color(0.5, 0.7, 1.0), t);
}

int main() {
    // Image dimensions
    const size_t width = 400;
    const size_t height = width / (16.0 / 9.0);
    // const size_t width = 256;
    // const size_t height = 256;

    const double aspect_ratio = width / height;

    // Viewport dimensions
    const double viewport_width = 2.0;
    const double viewport_height = viewport_width / aspect_ratio;

    // Unit vectors for viewport pixels
    const vec3 horizontal { viewport_width / width, 0.0, 0.0 };
    const vec3 vertical { 0.0, viewport_height / height, 0.0 };
    std::cerr << "Horizontal : " << horizontal << std::endl;
    std::cerr << "Vertical : " << vertical << std::endl;

    // We use a right handed coord system
    const point3 eye_pos {};
    const double focal_length = 1.0;

    const point3 viewport_first_pixel =
        horizontal * -(viewport_width / 2.0) +
        vertical * -(viewport_height / 2.0) +
        point3{ 0.0, 0.0, -focal_length };

    const point3 first_pixel = eye_pos + viewport_first_pixel;

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
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
