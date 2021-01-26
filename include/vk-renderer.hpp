#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vk-context.hpp"
#include "vulkan-loader.hpp"

class input_data;
class window;

class vkrenderer {
    public:
        vkrenderer(window& wnd, const input_data& inputs);

        ~vkrenderer();

        void compute(uint32_t width, uint32_t height);

        void recreate_swapchain();

        struct input_data*              mapped_data;

    private:
        void create_surface(window& wnd);

        void create_swapchain();

        void create_pipeline();

        void create_command_buffers();

        void create_descriptor_sets();

        void create_fences();

        void create_semaphores();

        void create_input_buffer(const input_data& inputs);

        void destroy_surface();

        void destroy_swapchain();

        void destroy_swapchain_images();

        void destroy_pipeline();

        void destroy_command_buffers();

        void destroy_descriptor_sets();

        void destroy_fences();

        void destroy_semaphores();

        void destroy_input_buffer();

        void update_descriptors_buffer();

        void update_descriptors_image();

        void handle_swapchain_result(VkResult function_result);

        vkcontext                       context;

        uint64_t                        frame_index = 0;

        VmaAllocation                   allocation;
        VkBuffer                        compute_shader_buffer;

        VkCommandPool                   command_pool;
        std::vector<VkCommandBuffer>    command_buffers;

        VkDescriptorPool                descriptor_pool;
        std::vector<VkDescriptorSet>    compute_shader_sets;

        VkShaderModule                  compute_shader_module;
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
};

#endif // !__VK_RENDERER_HPP_
