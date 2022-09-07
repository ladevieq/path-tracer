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

        static void queue_image_update(Texture* texture);

        static Buffer* create_buffer(size_t size);

        static Buffer* create_index_buffer(size_t size);

        static Buffer* create_staging_buffer(size_t size);

        static Texture* create_2d_texture(size_t width, size_t height, VkFormat format, Sampler *sampler = nullptr);

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

        void begin_frame();

        void finish_frame();


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

        const uint32_t                  min_swapchain_image_count = 3;

        static std::vector<Texture*>    upload_queue;
};

class RingBuffer {
    public:

    static const off_t invalid_alloc = -1;

    RingBuffer(size_t size);

    [[nodiscard]]off_t alloc(size_t alloc_size) const {
        assert(alloc_size > 0);

        if (ptr + alloc_size < buffer_size) {
            return ptr;
        }
        return invalid_alloc;
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

class Texture {
    public:

    Texture(handle image_handle) {
        auto image = vkrenderer::api.get_image(image_handle);
        width = image.size.width;
        height = image.size.height;
        depth = image.size.depth;

        device_image = image_handle;
    }

    Texture(size_t width, size_t height, size_t depth, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, Sampler* texture_sampler = nullptr)
        : width(width), height(height), depth(depth) {
        sampler = texture_sampler;
        device_image = vkrenderer::api.create_image(
            { .width = (uint32_t)width, .height = (uint32_t)height, .depth = 1 },
            format,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        );
    }

    void update(void* new_data) {
        if (data == nullptr) {
            free(data);
        }
        data = new_data;

        // vkrenderer::update_image(this);
    }

    size_t size() const {
        auto image = vkrenderer::api.get_image(device_image);
        return width * height * depth * pixel_size(image.format);
    }

    static size_t pixel_size(VkFormat format) {
        switch (format) {
            case VK_FORMAT_R8G8B8A8_UNORM:
                return 4;
            default:
                return 0;
        }
    }

    Sampler* sampler    = nullptr;

    void* data          = nullptr;

    size_t width        = 0;
    size_t height       = 0;
    size_t depth        = 0;

    handle device_image;
};
#endif // !__VK_RENDERER_HPP_
