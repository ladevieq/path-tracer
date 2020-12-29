#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "vec3.hpp"

struct input_data {
    color sky_color;
    color ground_color;

    float viewport_width;
    float viewport_height;
    float pad1;
    float pad2;
};
struct path_tracer_data {
    struct input_data inputs;
    color output_image[1920 * 1080];
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

        void fill_descriptor_set(const input_data& data);

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
