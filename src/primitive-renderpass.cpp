#include "primitive-renderpass.hpp"

#include "vulkan-loader.hpp"


void Primitive::set_pipeline(const char* shader_name) {
    std::vector<VkDynamicState> dynamic_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    pipeline = api.create_graphics_pipeline(shader_name, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), primitive_render_pass.render_pass, dynamic_states);
}

void Primitive::set_constant_offset(off_t offset) {
    constant_offset = offset;
}

void Primitive::set_constant(off_t offset, uint64_t* constant) {
    memcpy(constants + offset, constant, sizeof(uint64_t));
}

void Primitive::set_constant(off_t offset, Texture* texture) {
    auto image = api.get_image(texture->device_image);

    memcpy(constants + offset, (void*)&image.bindless_storage_index, sizeof(bindless_index));
}

void Primitive::set_constant(off_t offset, Buffer* buffer) {
    auto device_buffer = api.get_buffer(buffer->device_buffer);
    memcpy(constants + offset, (void*)&device_buffer.device_address, sizeof(VkDeviceAddress));
}

void Primitive::set_constant(off_t offset, Sampler* sampler) {
    auto device_sampler = api.get_sampler(sampler->device_sampler);
    memcpy(constants + offset, (void*)&device_sampler.bindless_index, sizeof(bindless_index));
}

void Primitive::set_index_buffer(Buffer* buffer) {
    this->index_buffer = buffer;
}

void Primitive::set_scissor(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
}

void Primitive::set_draw_info(size_t vertex_count, off_t first_vertex, off_t vertex_offset) {
    this->element_count = vertex_count;
    this->first_vertex = first_vertex;
    this->vertex_offset = vertex_offset;
}

void Primitive::render(VkCommandBuffer command_buffer) {
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    api.update_constants(command_buffer, VK_SHADER_STAGE_ALL_GRAPHICS, constant_offset, 64, (void*)&constants);

    api.draw(
        command_buffer,
        pipeline,
        index_buffer->device_buffer,
        element_count,
        first_vertex,
        vertex_offset
    );
};


PrimitiveRenderpass::PrimitiveRenderpass(vkapi& api)
        : Renderpass(api) {
    };

void PrimitiveRenderpass::add_primitive(Primitive* new_primitive) {
    primitives.push_back(new_primitive);
}

void PrimitiveRenderpass::add_color_attachment(VkFormat format) {
    color_attachments_format.push_back(format);
}

void PrimitiveRenderpass::set_viewport(float x, float y, float width, float height) {
    viewport.x = x;
    viewport.y = y;
    viewport.width = width;
    viewport.height = height;
}

void PrimitiveRenderpass::set_color_attachment(uint32_t index, Texture* attachment) {
    auto device_image = api.get_image(attachment->device_image);

    if (device_image.size.width != framebuffers[0].size.width ||
        device_image.size.height != framebuffers[0].size.height) {
        recreate_framebuffers();
    }


    color_attachments[index] = attachment;
    color_attachments_view[index] = device_image.view;
}

void PrimitiveRenderpass::finalize_render_pass() {
    render_pass = api.create_render_pass(color_attachments_format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    create_framebuffers();

    color_attachments.resize(color_attachments_format.size());
    color_attachments_view.resize(color_attachments_format.size());
}

void PrimitiveRenderpass::execute(vkrenderer& renderer, VkCommandBuffer command_buffer) {
    VkExtent2D extent = { .width = (uint32_t)viewport.width, .height = (uint32_t)viewport.height };

    for (auto& attachment : color_attachments) {
        api.image_barrier(command_buffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, attachment->device_image);
    }


    api.begin_render_pass(command_buffer, render_pass, color_attachments_view, framebuffers[renderer.frame_index()], extent);

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    for (auto primitive : primitives) {
        primitive->render(command_buffer);
    }

    api.end_render_pass(command_buffer);

    primitives.clear();
}

void PrimitiveRenderpass::create_framebuffers() {
    VkExtent2D size { .width = (uint32_t) viewport.width, .height = (uint32_t) viewport.height };

    for(auto& framebuffer : framebuffers) {
        framebuffer = api.create_framebuffer(render_pass, color_attachments_format, size);
    }
}

void PrimitiveRenderpass::recreate_framebuffers() {
    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }

    create_framebuffers();
}
