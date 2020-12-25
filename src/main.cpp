#include <iostream>
#include <vector>
#include <cstring>
#include <cstdio>

#include "vulkan-loader.hpp"
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

VkInstance          instance = nullptr;
VkPhysicalDevice    physical_device = nullptr;
int32_t             compute_queue_index = -1;
VkDevice            device = nullptr;
VkQueue             compute_queue = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT _severity,
                                             VkDebugUtilsMessageTypeFlagsEXT _type,
                                             const VkDebugUtilsMessengerCallbackDataEXT *_data,
                                             void *_userData)
{
    std::cerr << _data->pMessage << std::endl;
    return VK_FALSE;
}

void create_debugger(VkInstance instance) {
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengeCreateInfo {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        nullptr,
        0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        DebugCallback,
        nullptr,
    };

    vkCreateDebugUtilsMessengerEXT(
        instance,
        &debugUtilsMessengeCreateInfo,
        nullptr,
        &debugMessenger
    );
}

void create_instance() {
    VkApplicationInfo app_info   = {};
    app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName    = "path-tracer"; 
    app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName         = "gpu-path-tracer";
    app_info.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion          = VK_API_VERSION_1_0;


    const char* layers[1] = {
        "VK_LAYER_KHRONOS_validation",
    };
    const char* ext[1] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    VkInstanceCreateInfo instance_create_info       = {};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &app_info;
    instance_create_info.enabledLayerCount          = 1;
    instance_create_info.ppEnabledLayerNames        = layers;
    instance_create_info.enabledExtensionCount      = 1;
    instance_create_info.ppEnabledExtensionNames    = ext;

    vkCreateInstance(&instance_create_info, nullptr, &instance);

    load_instance_functions(instance);
    create_debugger(instance);
}

void select_physical_device() {
    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);

    if (physical_devices_count < 1) {
        std::cerr << "No physical device found !" << std::endl;
        exit(1);
    }

    std::vector<VkPhysicalDevice> physical_devices { physical_devices_count };
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data());

    physical_device = physical_devices[0];
}

int32_t get_compute_queue_index() {
    uint32_t queue_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties { queue_properties_count };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties.data());

    for (uint32_t properties_index = 0; properties_index < queue_properties_count; properties_index++) {
        if (queue_properties[properties_index].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            return properties_index;
        }
    }

    return -1;
}

void create_device() {
    select_physical_device();

    compute_queue_index                         = get_compute_queue_index();
    VkDeviceQueueCreateInfo queue_create_info   = {};
    queue_create_info.sType                     = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex          = compute_queue_index;
    queue_create_info.queueCount                = 1;

    float queue_priority                        = 1.f;
    queue_create_info.pQueuePriorities          = &queue_priority;

    VkPhysicalDeviceFeatures device_features    = {};

    VkDeviceCreateInfo device_create_info       = {};
    device_create_info.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos        = &queue_create_info;
    device_create_info.queueCreateInfoCount     = 1;
    device_create_info.pEnabledFeatures         = &device_features;
    device_create_info.enabledExtensionCount    = 0; // None yet
    device_create_info.enabledLayerCount        = 0; // https://www.khronos.org/registry/vulkan/specs/1.2/html/chap31.html#extendingvulkan-layers-devicelayerdeprecation

    vkCreateDevice(physical_device, &device_create_info, nullptr, &device);

    load_device_functions(device);

    vkGetDeviceQueue(device, compute_queue_index, 0, &compute_queue);
}

VkPipeline compute_pipeline;
void create_pipeline() {
    FILE* f = fopen("compute.spv", "r");
    if (f == nullptr) {
        std::cerr << "Cannot open compute shader file !" << std::endl;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        exit(1);
    }

    long len = ftell(f);
    const uint32_t* compute_shader_code = (uint32_t*) malloc(len);

    if (fseek(f, 0, SEEK_SET) != 0) {
        exit(1);
    }

    if (fread((void*) compute_shader_code, sizeof(uint32_t), len / sizeof(uint32_t), f) != len / sizeof(uint32_t)) {
        std::cerr << "Reading shader code failed" << std::endl;
        exit(1);
    }

    VkShaderModule compute_shader_module;
    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = nullptr;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = len;
    shader_create_info.pCode                        = compute_shader_code;

    vkCreateShaderModule(device, &shader_create_info, nullptr, &compute_shader_module);

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = nullptr;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = compute_shader_module;
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = nullptr;

    VkDescriptorSetLayoutBinding binding            = {};
    binding.binding                                 = 1;
    binding.descriptorType                          = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    binding.stageFlags                              = VK_SHADER_STAGE_COMPUTE_BIT;
    binding.pImmutableSamplers                      = 0;

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = nullptr;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = 1;
    descriptor_set_layout_create_info.pBindings     = &binding;

    VkDescriptorSetLayout layout;
    vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &layout);

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &layout;
    layout_create_info.pushConstantRangeCount       = 0;
    layout_create_info.pPushConstantRanges          = nullptr;

    VkPipelineLayout pipeline_layout;
    vkCreatePipelineLayout(device, &layout_create_info, nullptr, &pipeline_layout);

    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = nullptr;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = pipeline_layout;

    vkCreateComputePipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &compute_pipeline
    );
}

void init_vk() {
    create_instance();
    create_device();
    create_pipeline();
}

int main() {
    load_vulkan();
    init_vk();

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
