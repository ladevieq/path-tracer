#pragma once

#include <vector>

#include "renderpass.hpp"
#include "vk-renderer.hpp"

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

    void execute(vkrenderer& renderer, VkCommandBuffer command_buffer) final;

    VkRenderPass render_pass = VK_NULL_HANDLE;

    private:

    void create_framebuffers();

    void recreate_framebuffers();

    VkViewport viewport     = { .minDepth = 0.f, .maxDepth = 1.f };

    std::vector<Texture*> color_attachments;
    std::vector<VkImageView> color_attachments_view;
    std::vector<VkFormat> color_attachments_format;

    framebuffer framebuffers[vkrenderer::virtual_frames_count];

    std::vector<Primitive*> primitives;
};


class Primitive {
    public:

    Primitive(PrimitiveRenderpass& render_pass);

    void set_pipeline(const char* shader_name);

    void set_constant_offset(off_t offset);
    void set_constant(off_t offset, uint64_t* constant);
    void set_constant(off_t offset, Texture* texture);
    void set_constant(off_t offset, Buffer* buffer);
    void set_constant(off_t offset, Sampler* sampler);

    void set_index_buffer(Buffer* buffer);

    void set_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height);

    void set_draw_info(size_t vertex_count, off_t indices_off, off_t vertex_off);

    void render(VkCommandBuffer command_buffer);


private:

    vkapi& api;

    PrimitiveRenderpass& primitive_render_pass;

    pipeline    pipeline;

    Buffer*     index_buffer;

    size_t      element_count;
    off_t       indices_offset;
    off_t       vertex_offset;

    off_t       constant_offset;

    VkRect2D    scissor = {};

    uint8_t     constants[64];
};
