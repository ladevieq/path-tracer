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

        void begin_frame();

        void ui();

        void compute(uint32_t width, uint32_t height, bool clear);

        void finish_frame();

        void recreate_swapchain();

        Buffer                      compute_shader_buffer;

    private:

        void handle_swapchain_result(VkResult function_result);

        vkapi                           api;

        uint64_t                        frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        std::vector<Image>              accumulation_images;
        Image                           ui_texture;

        std::vector<VkCommandBuffer>    command_buffers;

        // TODO: Maybe use a map in the future
        std::vector<VkDescriptorSetLayoutBinding> compute_sets_bindings;
        std::vector<VkDescriptorSet>    compute_shader_sets;
        Pipeline                        compute_pipeline;

        std::vector<VkDescriptorSetLayoutBinding> ui_sets_bindings;
        std::vector<Buffer>             ui_transforms_buffers;
        std::vector<Buffer>             ui_vertex_buffers;
        std::vector<Buffer>             ui_index_buffers;
        std::vector<VkDescriptorSet>    ui_sets;
        Pipeline                        ui_pipeline;
        VkRenderPass                    render_pass;
        std::vector<VkFramebuffer>      framebuffers;

        std::vector<VkFence>            submission_fences;
        std::vector<VkSemaphore>        execution_semaphores;
        std::vector<VkSemaphore>        acquire_semaphores;

        VkSurfaceKHR                    platform_surface;
        Swapchain                       swapchain;

        const uint32_t                  min_swapchain_image_count = 3;
};

#endif // !__VK_RENDERER_HPP_
