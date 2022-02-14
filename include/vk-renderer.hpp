#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <stdint.h>
#include <vector>

#include "vk-api.hpp"
#include "vulkan/vulkan_core.h"


class Renderpass;
class ComputeRenderpass;
class window;

class Texture;
class Buffer;
class Sampler;

class vkrenderer {
    public:
        vkrenderer(window& wnd);

        ~vkrenderer();

        void begin_frame();

        void ui();

        void compute(uint32_t width, uint32_t height);

        void finish_frame();

        void recreate_swapchain();

        void update_image(Texture* texture, void* data, size_t size);

        Buffer* create_buffer(size_t size, bool isStatic);

        Texture* create_2d_texture(size_t width, size_t height, VkFormat format);

        Sampler* create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);

        ComputeRenderpass* create_compute_renderpass();

        void render();

        uint32_t frame_index() { return this->virtual_frame_index; }

    private:

        vkapi                           api;

        std::vector<Renderpass*>        renderpasses;


        void handle_swapchain_result(VkResult function_result);

        // vkapi                           api;

        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        sampler                         ui_texture_sampler;
        image                           ui_texture;

        std::vector<VkCommandBuffer>    command_buffers;

        buffer                          staging_buffer;


        pipeline                        tonemapping_pipeline;

        std::vector<buffer>             ui_vertex_buffers;
        std::vector<buffer>             ui_index_buffers;
        pipeline                        ui_pipeline;
        VkRenderPass                    render_pass;
        std::vector<VkFramebuffer>      framebuffers;

        std::vector<VkFence>            submission_fences;
        std::vector<VkSemaphore>        execution_semaphores;
        std::vector<VkSemaphore>        acquire_semaphores;

        VkSurfaceKHR                    platform_surface;
        swapchain                       swapchain;

        const uint32_t                  min_swapchain_image_count = 3;
        static constexpr uint32_t       virtual_frames_count = 2;
};

// TODO: Move those to there own files
class Texture {
    public:

    sampler sampler;

    image device_image;
};

class Sampler {
    public:
    sampler device_sampler;
};

class Buffer {
    public:

    Buffer(vkrenderer* renderer, size_t size, bool isStatic)
        : renderer(renderer), size(size), isStatic(isStatic) {}

    void* ptr() {
        size_t offset = isStatic ? 0 : size * renderer->frame_index();
        return (uint8_t*)device_buffer.alloc_info.pMappedData + offset;
    }

    unsigned long long device_address() {
        size_t offset = isStatic ? 0 : size * renderer->frame_index();
        return device_buffer.device_address + offset;
    }

    buffer device_buffer;

    private:

    bool isStatic;

    size_t size;

    vkrenderer* renderer;
};

#endif // !__VK_RENDERER_HPP_
