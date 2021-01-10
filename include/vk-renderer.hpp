#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "vec3.hpp"
#include "camera.hpp"
#include "sphere.hpp"
#include "window.hpp"

struct input_data {
    color sky_color;
    color ground_color;

    camera cam;

    float random_offset[2000];
    float random_disk[2000];
    vec3 random_in_sphere[1000];

    sphere spheres[512];

    uint32_t max_bounce;
    uint32_t samples_per_pixel;

    // Output image resolution
    uint32_t width;
    uint32_t height;
};

struct path_tracer_data {
    struct input_data inputs;
    color first_pixel;
};

class vkrenderer {
    public:
        vkrenderer(window& wnd);

        void compute(const input_data& inputs, size_t width, size_t height);

        const color* const output_image();

    private:
        void create_instance();

        void create_debugger();

        void create_surface(window& wnd);

        void create_device();

        void create_swapchain(window& wnd);

        void create_pipeline();

        void create_memory_allocator();

        void create_command_buffer();

        void create_descriptor_set();

        void create_semaphores();

        void select_physical_device();

        void select_compute_queue();

        void fill_descriptor_set(const input_data& data, size_t width, size_t height);

        uint32_t            compute_queue_index;

        VkInstance          instance;
        VkPhysicalDevice    physical_device;
        VkDevice            device;
        VkQueue             compute_queue;

        VmaAllocator        allocator;
        VkBuffer            compute_shader_buffer;

        VkCommandPool       command_pool;
        VkCommandBuffer     command_buffer;

        VkDescriptorPool    descriptor_pool;
        VkDescriptorSet     compute_shader_input_set;

        VkDescriptorSetLayout   compute_shader_input_layout;
        VkPipelineLayout        compute_pipeline_layout;
        VkPipeline              compute_pipeline;

        VkSemaphore             acquire_semaphore;

        VkSurfaceFormatKHR      surface_format;
        VkSurfaceKHR            platform_surface;
        VkSwapchainKHR          swapchain;
        size_t                  swapchain_images_count = 3;
        std::vector<VkImage>    swapchain_images;
        std::vector<VkImageView>swapchain_images_views;
        uint32_t                current_image_index;

        struct path_tracer_data* mapped_data;
};

#endif // !__VK_RENDERER_HPP_
