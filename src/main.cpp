#include <iostream>

#include "rt.hpp"
#include "hittable_list.hpp"

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

color ray_color(ray& r, const hittable& world, uint32_t depth) {
    if (depth <= 0) {
        return color{};
    }

    struct hit_info info;
    if (world.hit(r, 0.001, infinity, info)) {
        color attenuation;
        ray scattered {};

        if (info.mat_ptr->scatter(r, info, attenuation, scattered)) {
            return attenuation * ray_color(scattered, world, depth - 1);
        }
        return color{};
    }

    auto unit_dir = r.direction.unit();
    auto t = (unit_dir.y + 1.0) * 0.5;
    return lerp(ground_color, sky_color, t);
}

int main() {
    // Image dimensions
    const double aspect_ratio = 16.0 / 9.0;
    const size_t width = 1920;
    const size_t height = width / aspect_ratio;
    const uint32_t samples_per_pixel = 100;
    const uint32_t max_depth = 100;

    camera camera { { -2.0, 2.0, 1.0 }, { 0.0, 0.0, -1.0 }, 20.0, aspect_ratio };

    // World hittable objects
    hittable_list world {};

    auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
    auto material_center = std::make_shared<lambertian>(color(0.7, 0.3, 0.3));
    auto material_left   = std::make_shared<dielectric>(1.5);
    auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

    world.add(std::make_shared<sphere>(point3( 0.0, -100.5, -1.0), 100.0, material_ground));
    world.add(std::make_shared<sphere>(point3( 0.0,    0.0, -1.0),   0.5, material_center));
    world.add(std::make_shared<sphere>(point3(-1.0,    0.0, -1.0),   0.5, material_left));
    world.add(std::make_shared<sphere>(point3(-1.0,    0.0, -1.0),  -0.49, material_left));
    world.add(std::make_shared<sphere>(point3( 1.0,    0.0, -1.0),   0.5, material_right));

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

                pixel_color += ray_color(ray, world, max_depth);
            }

            write_color(std::cout, pixel_color, samples_per_pixel);
        }
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
