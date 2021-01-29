#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vk-api.hpp"
#include "vulkan-loader.hpp"

class input_data;
class window;

class vkrenderer {
    public:
        vkrenderer(window& wnd, const input_data& inputs);

        ~vkrenderer();

        void compute(uint32_t width, uint32_t height);

        void recreate_swapchain();

        Buffer                      compute_shader_buffer;

    private:

        void handle_swapchain_result(VkResult function_result);

        vkapi                           api;

        uint64_t                        frame_index = 0;

        std::vector<VkCommandBuffer>    command_buffers;

        VkDescriptorPool                descriptor_pool;
        std::vector<VkDescriptorSet>    compute_shader_sets;

        ComputePipeline                 compute_pipeline;

        std::vector<VkFence>            submission_fences;
        std::vector<VkSemaphore>        execution_semaphores;
        std::vector<VkSemaphore>        acquire_semaphores;

        VkSurfaceFormatKHR              surface_format;
        VkSurfaceKHR                    platform_surface = VK_NULL_HANDLE;

        Swapchain                       swapchain;

        const uint32_t                  min_swapchain_image_count = 3;
        uint32_t                        current_image_index;
};

#endif // !__VK_RENDERER_HPP_
