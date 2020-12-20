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
    camera camera { {}, aspect_ratio, 1.0 };

    // PPM format header
    std::cout << "P3\n" << width << '\n' << height << "\n 255" << std::endl;

    std::cerr << "Generating image" << std::endl;

    for (ssize_t row = height - 1; row >= 0; row--) {
        // std::cerr << row << " lines remaining" << std::endl;
        for (size_t column = 0; column < width; column++) {
            auto u = double(column) / (width - 1.0);
            auto v = double(row) / (height - 1.0);
            auto ray = camera.get_ray(u, v);

            write_color(std::cout, ray_color(ray, world), 1.0);
            // write_color(std::cout, color(ray.direction.unit().y, ray.direction.unit().y, ray.direction.unit().y));
            // write_color(std::cout, color(y.y, y.y, y.y));
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
