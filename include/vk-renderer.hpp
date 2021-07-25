#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

#include "vk-api.hpp"
#include "vulkan-loader.hpp"

class scene;
class window;

class vkrenderer {
    public:
        vkrenderer(window& wnd, size_t scene_buffer_size, size_t geometry_buffer_size, size_t bvh_buffer_size);

        ~vkrenderer();

        void begin_frame();

        void reset_accumulation();

        void ui();

        void compute(uint32_t width, uint32_t height);

        void finish_frame();

        void recreate_swapchain();

        inline void* scene_buffer_ptr() {
            return scene_buffer.alloc_info.pMappedData;
        }

        inline void* geometry_buffer_ptr() {
            return geometry_buffer.alloc_info.pMappedData;
        }

        inline void* bvh_buffer_ptr() {
            return bvh_buffer.alloc_info.pMappedData;
        }

    private:

        void handle_swapchain_result(VkResult function_result);

        vkapi                           api;

        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        std::vector<Image>              accumulation_images;
        VkSampler                       ui_texture_sampler;
        Image                           ui_texture;

        std::vector<VkCommandBuffer>    command_buffers;

        // TODO: Maybe use a map in the future
        std::vector<VkDescriptorSetLayoutBinding> compute_sets_bindings;
        std::vector<VkDescriptorSet>    compute_shader_sets;
        Pipeline                        compute_pipeline;

        Buffer                          scene_buffer;
        Buffer                          geometry_buffer;
        Buffer                          bvh_buffer;


        std::vector<VkDescriptorSetLayoutBinding> tonemapping_sets_bindings;
        std::vector<VkDescriptorSet>    tonemapping_shader_sets;
        Pipeline                        tonemapping_pipeline;

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
        const uint32_t                  virtual_frames_count = 2;
};

#endif // !__VK_RENDERER_HPP_
