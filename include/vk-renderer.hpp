#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "vec3.hpp"

struct input_data {
    color sky_color;
    color ground_color;
    vec3 camera_pos;

    uint32_t samples_per_pixel;
    float viewport_width;
    float viewport_height;
    float proj_plane_distance;

    float random_numbers[200];

    struct sphere {
        vec3 position;
        float radius;
        float padding[3];
    } sphere;
};

struct path_tracer_data {
    struct input_data inputs;
    color first_pixel;
};

class vkrenderer {
    public:
        vkrenderer();

        void compute(const input_data& inputs, size_t width, size_t height);

        const color* const output_image();

    private:
        void create_instance();

        void create_debugger();

        void create_device();

        void create_pipeline();

        void create_memory_allocator();

        void create_command_buffer();

        void create_descriptor_set();

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

        struct path_tracer_data* mapped_data;
};

#endif // !__VK_RENDERER_HPP_
