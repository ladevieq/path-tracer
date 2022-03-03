#pragma once

#include <stdint.h>
#include <vector>
#include <array>

#include "vk-renderer.hpp"
#include "renderpass.hpp"

class Buffer;
class Texture;
class Sampler;
class Primitive;

class PrimitiveRenderpass : public Renderpass {
    public:

    PrimitiveRenderpass(vkapi& api);

    void add_primitive(Primitive* new_primitive);

    void add_color_attachment(VkFormat format);

    void set_depth_stencil(Texture* depth_stencil);

    void set_viewport(float x, float y, float width, float height);

    void set_color_attachment(uint32_t index, Texture* attachment);

    void finalize_render_pass();

    void execute(vkrenderer& renderer, VkCommandBuffer command_buffer) override final;

    VkRenderPass render_pass = VK_NULL_HANDLE;

    private:

    void create_framebuffers();

    void recreate_framebuffers();

    VkViewport viewport     = { .minDepth = 0.f, .maxDepth = 1.f };

    std::vector<Texture*> color_attachments;
    std::vector<VkImageView> color_attachments_view;
    std::vector<VkFormat> color_attachments_format;

    std::array<framebuffer, vkrenderer::virtual_frames_count> framebuffers;

    std::vector<Primitive*> primitives;
};


class Primitive {
    public:

    Primitive(vkapi& api, PrimitiveRenderpass& render_pass)
        : api(api), primitive_render_pass(render_pass)
    {};

    void set_pipeline(std::string& shader_name);

    void set_constant_offset(off_t offset);
    void set_constant(off_t offset, uint64_t* constant);
    void set_constant(off_t offset, Texture* texture);
    void set_constant(off_t offset, Buffer* buffer);
    void set_constant(off_t offset, Sampler* sampler);

    void set_index_buffer(Buffer* index_buffer);

    void set_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

    void set_draw_info(size_t vertex_count, off_t first_vertex, off_t vertex_offset);

    void render(VkCommandBuffer command_buffer);


private:

    vkapi& api;

    PrimitiveRenderpass& primitive_render_pass;

    pipeline    pipeline;

    Buffer*     index_buffer;

    size_t      element_count;
    off_t       first_vertex;
    off_t       vertex_offset;

    off_t       constant_offset;

    VkRect2D    scissor = {};

    std::array<uint8_t, 64> constants;
};
