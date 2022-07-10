#ifndef __VK_RENDERER_HPP_
#define __VK_RENDERER_HPP_

#include <vector>

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

        void update_buffer(Buffer* buffer, void* data, off_t offset, size_t size);

        Buffer* create_buffer(size_t size, bool isStatic);

        Buffer* create_index_buffer(size_t size, bool isStatic);

        Texture* create_2d_texture(size_t width, size_t height, VkFormat format);

        Sampler* create_sampler(VkFilter filter, VkSamplerAddressMode address_mode);

        ComputeRenderpass* create_compute_renderpass();

        PrimitiveRenderpass* create_primitive_renderpass();

        Primitive* create_primitive(PrimitiveRenderpass& primitive_render_pass);

        void render();

        [[nodiscard]]uint32_t frame_index() const { return this->virtual_frame_index; }

        [[nodiscard]]Texture* back_buffer() const { return swapchain_textures[swapchain_image_index]; }

        [[nodiscard]]VkFormat back_buffer_format() const { return swapchain.surface_format.format; }

        static constexpr uint32_t       virtual_frames_count = 2;

        vkcontext context;

        vkapi                           api;

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

    Buffer(size_t size, bool isStatic)
        : size(size), isStatic(isStatic) {}

    handle device_buffer;

    size_t size;

    bool isStatic;
};

#endif // !__VK_RENDERER_HPP_
