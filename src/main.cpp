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
    const uint32_t samples_per_pixel = 100;

    camera camera { {}, aspect_ratio, 1.0 };

    // World hittable objects
    hittable_list world {};
    world.add(std::make_shared<sphere>(point3(0,0,-1), 0.5));
    world.add(std::make_shared<sphere>(point3(0,-100.5,-1), 100));

    // PPM format header
    std::cout << "P3\n" << width << '\n' << height << "\n 255" << std::endl;

    std::cerr << "Generating image" << std::endl;

    for (ssize_t row = height - 1; row >= 0; row--) {
        std::cerr << row << " lines remaining" << std::endl;
        for (size_t column = 0; column < width; column++) {
            color pixel_color;

            for (size_t sample_index = 0; sample_index < samples_per_pixel; sample_index++) {
                auto u = double(column + randd()) / (width - 1.0);
                auto v = double(row + randd()) / (height - 1.0);
                auto ray = camera.get_ray(u, v);

                pixel_color += ray_color(ray, world);
            }

            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
