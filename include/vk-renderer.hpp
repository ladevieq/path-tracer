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
class RingBuffer;

class vkrenderer {
    public:
        vkrenderer(window& wnd);

        ~vkrenderer();

        void recreate_swapchain();


        static Buffer* create_buffer(size_t size);

        static Buffer* create_index_buffer(size_t size);

        static Texture* create_2d_texture(size_t width, size_t height, VkFormat format, Sampler *sampler = nullptr);

        static void destroy_2d_texture(Texture* texture);

        static Sampler* create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);


        ComputeRenderpass* create_compute_renderpass();

        PrimitiveRenderpass* create_primitive_renderpass();

        Primitive* create_primitive(PrimitiveRenderpass& primitive_render_pass);

        void render();

        RingBuffer* get_staging_buffer() { return staging_buffer; }

        [[nodiscard]]uint32_t frame_index() const { return this->virtual_frame_index; }

        [[nodiscard]]Texture* back_buffer() const { return swapchain_textures[swapchain_image_index]; }

        [[nodiscard]]VkFormat back_buffer_format() const { return swapchain.surface_format.format; }

        static constexpr uint32_t       virtual_frames_count = 2;

        static vkcontext                context;

        static vkapi                    api;

    private:

        void begin_frame();

        void finish_frame();


        void handle_swapchain_result(VkResult function_result);

        std::vector<Renderpass*>        renderpasses;

        static constexpr uint32_t       initial_copy_command_buffers = 10U;
        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        VkCommandBuffer                 graphics_command_buffers[virtual_frames_count];
        std::vector<VkCommandBuffer>    copy_command_buffers[virtual_frames_count];

        RingBuffer*                     staging_buffer;


        pipeline                        tonemapping_pipeline;

        VkFence                         submission_fences[virtual_frames_count];
        VkSemaphore                     execution_semaphores[virtual_frames_count];
        VkSemaphore                     acquire_semaphores[virtual_frames_count];

        VkCommandPool                   graphics_command_pool;
        VkCommandPool                   copy_command_pools[virtual_frames_count];

        VkSurfaceKHR                    platform_surface;
        swapchain                       swapchain;
        std::vector<Texture*>           swapchain_textures;

        const uint32_t                  min_swapchain_image_count = 3;
};

class RingBuffer {
    public:
    RingBuffer(size_t size)
        : buffer_size(size) {}

    [[nodiscard]]size_t alloc(size_t alloc_size) const {
        assert(alloc_size > 0);

        if (ptr + alloc_size < buffer_size) {
            return ptr;
        }
        return 0;
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

class Sampler {
    public:

    handle device_sampler;
};

// TODO: Move those to there own files
class Texture {
    public:

    void write(void* data) const {
        auto image = vkrenderer::api.get_image(device_image);
        auto texture_size = image.size.width * image.size.height * 4;

        // TODO: Do all this inside vkrenderer
        // auto* buffer =  vkrenderer::get_staging_buffer();
        // auto alloc_offset = buffer->alloc(texture_size);
        // assert(alloc_offset > 0);

        // // std::memcpy(buffer.device_ptr, data, texture_size);
        // buffer->write(data, alloc_offset, texture_size);

        // vkrenderer::api.create_command_buffers
        // submit upload work
        // VKRESULT(vkWaitForFences(vkrenderer::context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
        // VKRESULT(vkResetFences(vkrenderer::context.device, 1, &submission_fences[virtual_frame_index]))

        // auto* cmd_buf = command_buffers[virtual_frame_index];
        // api.start_record(cmd_buf);

        // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, device_image);

        // api.copy_buffer(cmd_buf, buffer, device_image);

        // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, device_image);

        // api.end_record(cmd_buf);



        // VKRESULT(vkWaitForFences(vkrenderer::context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    }

    Sampler* sampler = nullptr;

    handle device_image;
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

#endif // !__VK_RENDERER_HPP_
