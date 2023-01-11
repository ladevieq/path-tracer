#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <cstdint>
#include <format>
#include <vector>

#include "vk-context.hpp"
#include "vk-api.hpp"


class Renderpass;
class ComputeRenderpass;
class PrimitiveRenderpass;
class Primitive;
class window;

class Texture;
class Sampler;
class RingBuffer;


class Buffer {
    public:

    Buffer(size_t size, VkBufferUsageFlagBits usage);
    ~Buffer();

    void write(void* data, off_t alloc_offset, size_t data_size) const;

    void resize(size_t new_size);

    handle device_buffer;

    size_t buffer_size;

    VkBufferUsageFlagBits usage;
};


class vkrenderer {
    public:
        vkrenderer(window& wnd);

        ~vkrenderer();

        void recreate_swapchain();


        void update_images();

        void begin_frame();

        void finish_frame();

        static void queue_image_update(Texture* texture);

        static Buffer* create_buffer(size_t size);

        static Buffer* create_index_buffer(size_t size);

        static Buffer* create_staging_buffer(size_t size);

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

        std::vector<Renderpass*>        renderpasses;

        uint32_t                        virtual_frame_index = 0;
        uint32_t                        swapchain_image_index = 0;

        VkCommandBuffer                 graphics_command_buffers[virtual_frames_count];
        VkCommandBuffer                 copy_command_buffers[virtual_frames_count];

        RingBuffer*                     staging_buffers[virtual_frames_count];


        pipeline                        tonemapping_pipeline;

        VkFence                         submission_fences[virtual_frames_count];
        VkSemaphore                     execution_semaphores[virtual_frames_count];
        VkSemaphore                     acquire_semaphores[virtual_frames_count];

        VkCommandPool                   graphics_command_pool;
        VkCommandPool                   copy_command_pools[virtual_frames_count];

        VkSurfaceKHR                    platform_surface;
        swapchain                       swapchain;
        std::vector<Texture*>           swapchain_textures;
        std::vector<VkCommandBuffer>    recorded_command_buffers;

        const uint32_t                  min_swapchain_image_count = 3;

        static std::vector<Texture*>    upload_queue;
};

class RingBuffer {
    public:

    static const size_t invalid_alloc = -1;

    RingBuffer(size_t size);

    [[nodiscard]]size_t alloc(size_t alloc_size) {
        assert(alloc_size > 0 && alloc_size <= buffer_size);

        auto alloc_offset = ptr;

        if (alloc_offset <= buffer_size - alloc_size) {
            ptr += alloc_size;
            return alloc_offset;
        }

        return invalid_alloc;
    }

    void write(void* data, size_t alloc_offset, size_t data_size) const {
        auto api_buffer = vkrenderer::api.get_buffer(device_buffer);

        std::memcpy((uint8_t*)api_buffer.device_ptr + alloc_offset, data, data_size);
    }

    void reset() {
        ptr = 0;
    }

    handle device_buffer;

    size_t buffer_size;
    size_t ptr = 0;
};

class Sampler {
    public:

    handle device_sampler;
};

#endif // !__VK_RENDERER_HPP_
