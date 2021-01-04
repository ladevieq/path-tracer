#include <iostream>
#include <vector>
#include <cstring>
#include <cstdio>

#include "vk-renderer.hpp"

#include "rt.hpp"

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
    vkrenderer renderer {};

    // Image dimensions
    const float aspect_ratio = 16.0 / 9.0;
    // const float aspect_ratio = 1.0;
    const size_t width = 1920;
    const size_t height = width / aspect_ratio;
    const uint32_t samples_per_pixel = 100;
    const uint32_t max_depth = 100;

    // const vec3 camera_position{ 13.0, 2.0, 3.0 };
    const vec3 camera_position{ 0.0, 0.0, 0.0 };
    const vec3 camera_target{ 0.0, 0.0, 1.0 };
    const double aperture = 0.1;
    const double distance_to_focus = 10;
    camera camera { camera_position, camera_target, 20.0, aspect_ratio, aperture, distance_to_focus };
    struct input_data inputs = {
        .sky_color = sky_color,
        .ground_color = ground_color,
        .camera_pos = camera_position,
        .samples_per_pixel = samples_per_pixel,
        .viewport_width = 2.f * aspect_ratio,
        .viewport_height = 2.f,
        .proj_plane_distance = 1.f,
        .spheres = { 
            { { 0.f, 0.f, -1.f },       { { 0.7f, 0.7f, 0.3f }, },  0.5f },
            { { 0.f, -100.5f, -1.f },   { { 0.8f, 0.8f, 0.f }, },   100.f }
        },
        .max_bounce = max_depth,
    };

    for (size_t rand_number_index = 0; rand_number_index < samples_per_pixel * 2; rand_number_index++) {
        inputs.random_numbers[rand_number_index] = randd();
    }

    for (size_t rand_number_index = 0; rand_number_index < max_depth * 10; rand_number_index++) {
        inputs.random_in_sphere[rand_number_index] = random_in_unit_sphere();
    }

    // World hittable objects
    hittable_list world = random_scene();

    // PPM format header
    std::cout << "P3\n" << width << '\n' << height << "\n255" << std::endl;

    std::cerr << "Generating image" << std::endl;

    renderer.compute(inputs, width, height);

    //-------------------------
    // GPU path tracer
    //-------------------------
    auto output_image = renderer.output_image();
    for (size_t index = 0; index < height * width; index++) {
        write_color(std::cout, output_image[index]);
    }

    std::cerr << "Done !" << std::endl;

    return 0;
}
