#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <cstdint>
#include <vector>

#include "vk-context.hpp"
#include "vk-api.hpp"


class Renderpass;
class ComputeRenderpass;
class PrimitiveRenderpass;
class Primitive;
class window;

class Texture;
class Buffer;
class Sampler;

class vkrenderer {
    public:
        vkrenderer(window& wnd);

        ~vkrenderer();

        void begin_frame();

        void finish_frame();

        void recreate_swapchain();

        void update_image(Texture* texture, void* data);

        static Buffer* create_buffer(size_t size);

        static Buffer* create_index_buffer(size_t size);

        static Texture* create_2d_texture(size_t width, size_t height, VkFormat format);

        static void destroy_2d_texture(Texture* texture);

        static Sampler* create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);

        ComputeRenderpass* create_compute_renderpass();

        PrimitiveRenderpass* create_primitive_renderpass();

        Primitive* create_primitive(PrimitiveRenderpass& primitive_render_pass);

        void render();

        [[nodiscard]]uint32_t frame_index() const { return this->virtual_frame_index; }

        [[nodiscard]]Texture* back_buffer() const { return swapchain_textures[swapchain_image_index]; }

        [[nodiscard]]VkFormat back_buffer_format() const { return swapchain.surface_format.format; }

        static constexpr uint32_t       virtual_frames_count = 2;

        static vkcontext                context;

        static vkapi                    api;

    private:

        void handle_swapchain_result(VkResult function_result);

        // vkapi                           api;

        std::vector<Renderpass*>        renderpasses;

        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        std::vector<VkCommandBuffer>    command_buffers;

        handle                          staging_buffer;


        pipeline                        tonemapping_pipeline;

        VkFence                         submission_fences[virtual_frames_count];
        VkSemaphore                     execution_semaphores[virtual_frames_count];
        VkSemaphore                     acquire_semaphores[virtual_frames_count];

        VkSurfaceKHR                    platform_surface;
        swapchain                       swapchain;
        std::vector<Texture*>           swapchain_textures;

        const uint32_t                  min_swapchain_image_count = 3;
};

// TODO: Move those to there own files
class Texture {
    public:

    sampler sampler;

    handle device_image;
};

class Sampler {
    public:

    handle device_sampler;
};

class Buffer {
    public:

    Buffer(size_t size);

    ~Buffer() {
        vkrenderer::api.destroy_buffer(device_buffer);
    }

    void write(void* data, off_t alloc_offset, size_t data_size) const {
        auto api_buffer = vkrenderer::api.get_buffer(device_buffer);

        std::memcpy((uint8_t*)api_buffer.device_ptr + alloc_offset, data, data_size);
    }

    handle device_buffer;

    size_t buffer_size;
};

class RingBuffer {
    public:
    RingBuffer(size_t size)
        : buffer_size(size) {}

    [[nodiscard]]bool alloc(size_t alloc_size) const {
        return ptr + alloc_size > buffer_size;
    }

    void write(void* data, off_t alloc_offset, size_t data_size) const {
        auto api_buffer = vkrenderer::api.get_buffer(device_buffer);

        std::memcpy((uint8_t*)api_buffer.device_ptr + alloc_offset, data, data_size);
    }

    void reset() {
        ptr = 0;
    }

    handle device_buffer;

    size_t buffer_size;
    off_t ptr;
};

#endif // !__VK_RENDERER_HPP_
