#include "compute-renderpass.hpp"

#include "vk-api.hpp"

#include "vk-renderer.hpp"

void ComputeRenderpass::set_ouput_texture(Texture* output_texture) {
    this->output_texture = output_texture;

    // TODO: Remove hardcoded offset
    memcpy(constants.data() + 56, &output_texture->device_image.bindless_storage_index, 4);
}

void ComputeRenderpass::set_dispatch_size(size_t count_x, size_t count_y, size_t count_z) {
    group_count_x = count_x;
    group_count_y = count_y;
    group_count_z = count_z;
}

void ComputeRenderpass::set_constant(off_t offset, uint64_t* constant) {
    memcpy(constants.data() + offset, constant, sizeof(uint64_t));
}

void ComputeRenderpass::set_constant(off_t offset, Texture* texture) {
    memcpy(constants.data() + offset, (void*)&texture->device_image.bindless_storage_index, 4);

    input_textures.push_back(texture);
}

void ComputeRenderpass::execute(VkCommandBuffer command_buffer) {
    vkCmdPushConstants(command_buffer, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 64, (void*)&constants);

    for (auto& texture: input_textures) {
        api.image_barrier(command_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT, texture->device_image);
    }

    api.image_barrier(command_buffer, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, output_texture->device_image);

    api.run_compute_pipeline(command_buffer, pipeline, group_count_x, group_count_y, group_count_z);

    input_textures.clear();
}
