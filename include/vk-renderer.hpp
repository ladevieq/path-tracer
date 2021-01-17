#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "window.hpp"

class input_data;

class vkrenderer {
    public:
        vkrenderer(window& wnd, const input_data& inputs);

        void compute();

        void recreate_swapchain();

    private:
        void create_instance();

        void create_debugger();

        void create_surface();

        void create_device();

        void create_swapchain();

        void create_pipeline();

        void create_memory_allocator();

        void create_command_buffers();

        void create_descriptor_set();

        void create_fences();

        void create_semaphores();

        void create_input_buffer(const input_data& inputs);

        void destroy_swapchain();

        void select_physical_device();

        void select_compute_queue();

        void update_descriptors_buffer();

        void update_descriptors_image();

        void handle_swapchain_result(VkResult function_result);

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
        VkSurfaceKHR                    platform_surface = VK_NULL_HANDLE;
        VkSwapchainKHR                  swapchain = VK_NULL_HANDLE;
        size_t                          swapchain_images_count = 3;
        std::vector<VkImage>            swapchain_images;
        std::vector<VkImageView>        swapchain_images_views;
        uint32_t                        current_image_index;

        struct input_data*              mapped_data;

        window&                         wnd;
};

#endif // !__VK_RENDERER_HPP_
