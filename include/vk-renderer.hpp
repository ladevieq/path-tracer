#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "window.hpp"
#include "camera.hpp"
#include "sphere.hpp"

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

class vkrenderer {
    public:
        vkrenderer(window& wnd, const input_data& inputs);

        void compute(size_t width, size_t height);

    private:
        void create_instance();

        void create_debugger();

        void create_surface(window& wnd);

        void create_device();

        void create_swapchain(window& wnd);

        void create_pipeline();

        void create_memory_allocator();

        void create_command_buffers();

        void create_descriptor_set();

        void create_fences();

        void create_semaphores();

        void select_physical_device();

        void select_compute_queue();

        void update_sets(const input_data& data);

        uint32_t                        compute_queue_index;
        uint64_t                        frame_index = 0;

        VkInstance                      instance;
        VkPhysicalDevice                physical_device;
        VkDevice                        device;
        VkQueue                         compute_queue;

        VmaAllocator                    allocator;
        VkBuffer                        compute_shader_buffer;

        VkCommandPool                   command_pool;
        std::vector<VkCommandBuffer>    command_buffers;

        VkDescriptorPool                descriptor_pool;
        std::vector<VkDescriptorSet>    compute_shader_sets;

        VkDescriptorSetLayout           compute_shader_layout;
        VkPipelineLayout                compute_pipeline_layout;
        VkPipeline                      compute_pipeline;

        std::vector<VkFence>            submission_fences;
        std::vector<VkSemaphore>        execution_semaphores;
        std::vector<VkSemaphore>        acquire_semaphores;

        VkSurfaceFormatKHR              surface_format;
        VkSurfaceKHR                    platform_surface;
        VkSwapchainKHR                  swapchain;
        size_t                          swapchain_images_count = 3;
        std::vector<VkImage>            swapchain_images;
        std::vector<VkImageView>        swapchain_images_views;
        uint32_t                        current_image_index;

        struct input_data*      mapped_data;
};

#endif // !__VK_RENDERER_HPP_
