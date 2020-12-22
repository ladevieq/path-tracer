#include <iostream>

#include "rt.hpp"
#include "hittable_list.hpp"

color ground_color { 1.0, 1.0, 1.0 };
color sky_color { 0.5, 0.7, 1.0 };

hittable_list random_scene() {
    hittable_list world;

    auto ground_material = std::make_shared<lambertian>(color{ 0.5, 0.5, 0.5 });
    world.add(std::make_shared<sphere>(point3{ 0,-1000,0 }, 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = randd();
            point3 center(a + 0.9 * randd(), 0.2, b + 0.9 * randd());

            if ((center - point3{ 4, 0.2, 0 }).length() > 0.9) {
                std::shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = std::make_shared<lambertian>(albedo);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = randd(0, 0.5);
                    sphere_material = std::make_shared<metal>(albedo, fuzz);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = std::make_shared<dielectric>(1.5);
                    world.add(std::make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = std::make_shared<dielectric>(1.5);
    world.add(std::make_shared<sphere>(point3{ 0, 1, 0 }, 1.0, material1));

    auto material2 = std::make_shared<lambertian>(color{ 0.4, 0.2, 0.1 });
    world.add(std::make_shared<sphere>(point3{ -4, 1, 0 }, 1.0, material2));

    auto material3 = std::make_shared<metal>(color{ 0.7, 0.6, 0.5 }, 0.0);
    world.add(std::make_shared<sphere>(point3{ 4, 1, 0 }, 1.0, material3));

    return world;
}

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
    const uint32_t samples_per_pixel = 1000;
    const uint32_t max_depth = 100;

    const vec3 camera_position{ 13.0, 2.0, 3.0 };
    const vec3 camera_target{ 0.0, 0.0, 0.0 };
    const double aperture = 0.1;
    const double distance_to_focus = 10;
    camera camera { camera_position, camera_target, 20.0, aspect_ratio, aperture, distance_to_focus };

    // World hittable objects
    hittable_list world = random_scene();

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
