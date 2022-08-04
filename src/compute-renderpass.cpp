#include "compute-renderpass.hpp"

#include "vk-renderer.hpp"

ComputeRenderpass::ComputeRenderpass(vkapi& api)
    : Renderpass(api)
{}

ComputeRenderpass::~ComputeRenderpass() {
    api.destroy_pipeline(pipeline);
}

void ComputeRenderpass::set_pipeline(const char* shader_name) {
    if (this->pipeline.handle != VK_NULL_HANDLE)
        api.destroy_pipeline(this->pipeline);
    this->pipeline = api.create_compute_pipeline(shader_name);
}

void ComputeRenderpass::set_ouput_texture(Texture* out_texture) {
    this->output_texture = out_texture;

    auto image = api.get_image(output_texture->device_image);

    // TODO: Remove hardcoded offset
    memcpy(constants + 56, &image.bindless_storage_index, sizeof(bindless_index));
}

void ComputeRenderpass::set_dispatch_size(size_t count_x, size_t count_y, size_t count_z) {
    group_count_x = count_x;
    group_count_y = count_y;
    group_count_z = count_z;
}

void ComputeRenderpass::set_constant(off_t offset, uint64_t* constant) {
    memcpy(constants + offset, constant, sizeof(uint64_t));
}

void ComputeRenderpass::set_constant(off_t offset, Texture* texture) {
    auto image = api.get_image(texture->device_image);

    memcpy(constants + offset, (void*)&image.bindless_storage_index, sizeof(bindless_index));

    input_textures.push_back(texture);
}

void ComputeRenderpass::set_constant(off_t offset, Buffer* buffer) {
    auto device_buffer = api.get_buffer(buffer->device_buffer);
    memcpy(constants + offset, (void*)&device_buffer.device_address, sizeof(VkDeviceAddress));
}

void ComputeRenderpass::execute(vkrenderer& renderer, VkCommandBuffer command_buffer) {
    api.update_constants(command_buffer, VK_SHADER_STAGE_COMPUTE_BIT, 0, 64, (void*)&constants);

    for (auto& texture: input_textures) {
        api.image_barrier(command_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, texture->device_image);
    }

    api.image_barrier(command_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, output_texture->device_image);

    api.run_compute_pipeline(command_buffer, pipeline, group_count_x, group_count_y, group_count_z);

    input_textures.clear();
}
