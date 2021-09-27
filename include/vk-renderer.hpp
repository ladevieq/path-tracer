#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vk-api.hpp"
#include "vulkan-loader.hpp"

class scene;
class window;

class vkrenderer {
    public:
        vkrenderer(window& wnd);

        ~vkrenderer();

        void begin_frame();

        void reset_accumulation();

        void ui();

        void compute(uint32_t width, uint32_t height);

        void finish_frame();

        void recreate_swapchain();

        void update_image(image img, void* data, size_t size);

        vkapi                           api;

        VkDeviceAddress                 scene_buffer_addr;
        VkDeviceAddress                 indices_buffer_addr;
        VkDeviceAddress                 positions_buffer_addr;
        VkDeviceAddress                 normals_buffer_addr;
        VkDeviceAddress                 uvs_buffer_addr;
        VkDeviceAddress                 bvh_buffer_addr;
        VkDeviceAddress                 materials_buffer_addr;

    private:

        void handle_swapchain_result(VkResult function_result);

        // vkapi                           api;

        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        std::vector<image>              accumulation_images;
        sampler                         ui_texture_sampler;
        image                           ui_texture;

        std::vector<VkCommandBuffer>    command_buffers;

        Pipeline                        compute_pipeline;

        buffer                          staging_buffer;


        Pipeline                        tonemapping_pipeline;

        std::vector<buffer>             ui_vertex_buffers;
        std::vector<buffer>             ui_index_buffers;
        Pipeline                        ui_pipeline;
        VkRenderPass                    render_pass;
        std::vector<VkFramebuffer>      framebuffers;

        std::vector<VkFence>            submission_fences;
        std::vector<VkSemaphore>        execution_semaphores;
        std::vector<VkSemaphore>        acquire_semaphores;

        VkSurfaceKHR                    platform_surface;
        swapchain                       swapchain;

        const uint32_t                  min_swapchain_image_count = 3;
        const uint32_t                  virtual_frames_count = 2;
};

#endif // !__VK_RENDERER_HPP_
