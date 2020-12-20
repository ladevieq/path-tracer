#include <iostream>

#include "rt.hpp"
#include "hittable_list.hpp"

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

color ray_color(ray& r, const hittable& world) {
    sphere sphere { { 0.0, 0.0, -1.0 }, 0.5 };

    auto unit_dir = r.direction.unit();
    auto t = (unit_dir.y + 1.0) * 0.5;

    auto c = lerp(ground_color, sky_color, t);

    struct hit_info info;
    if (world.hit(r, 0.0, infinity, info)) {
        c = 0.5 * (info.normal + vec3{ 1.0, 1.0, 1.0});
    }

    return c;
}

int main() {
    // Image dimensions
    const double aspect_ratio = 16.0 / 9.0;
    const size_t width = 400;
    const size_t height = width / aspect_ratio;

    // World hittable objects
    hittable_list world {};
    world.add(std::make_shared<sphere>(point3(0,0,-1), 0.5));
    world.add(std::make_shared<sphere>(point3(0,-100.5,-1), 100));

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

            write_color(std::cout, ray_color(ray, world));
            // write_color(std::cout, color(direction.unit().y, direction.unit().y, direction.unit().y));
            // write_color(std::cout, color(y.y, y.y, y.y));
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
