#include "vk-renderer.hpp"

#include <cstring>
#include <cassert>

#include <vk_mem_alloc.h>

#include "vulkan-loader.hpp"

#include "compute-renderpass.hpp"
#include "primitive-renderpass.hpp"

#include "window.hpp"

#ifdef _DEBUG
#define VKRESULT(result) assert(result == VK_SUCCESS);
#else
#define VKRESULT(result) result;
#endif

vkrenderer::vkrenderer(window& wnd)
    : api(context) {
    platform_surface = api.create_surface(wnd);
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_NULL_HANDLE
    );

    swapchain_textures.resize(swapchain.image_count);
    for (size_t index = 0; index < swapchain.image_count; index++) {
        swapchain_textures[index] = new Texture();
        swapchain_textures[index]->device_image = swapchain.images[index];
    }

    tonemapping_pipeline = api.create_compute_pipeline("tonemapping");

    command_buffers = api.create_command_buffers(virtual_frames_count);

    for(size_t index { 0 }; index < virtual_frames_count; index++) {
        submission_fences[index] = api.create_fence();
        execution_semaphores[index] = api.create_semaphore();
        acquire_semaphores[index] = api.create_semaphore();
    }

    staging_buffer = api.create_buffer(4096 * 4096 * 4 * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(context.device, virtual_frames_count, submission_fences, VK_TRUE, UINT64_MAX))

    for (auto& renderpass : renderpasses) {
        delete renderpass;
    }

    api.destroy_fences(submission_fences, virtual_frames_count);
    api.destroy_semaphores(execution_semaphores, virtual_frames_count);
    api.destroy_semaphores(acquire_semaphores, virtual_frames_count);

    api.destroy_command_buffers(command_buffers);

    api.destroy_pipeline(tonemapping_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::begin_frame() {
    VKRESULT(vkWaitForFences(context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(context.device, 1, &submission_fences[virtual_frame_index]))

    swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[virtual_frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    api.start_record(command_buffers[virtual_frame_index]);
}

void vkrenderer::finish_frame() {
    auto* cmd_buf = command_buffers[virtual_frame_index];

    // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, swapchain.images[swapchain_image_index]);

    api.end_record(cmd_buf);

    api.submit(cmd_buf, acquire_semaphores[virtual_frame_index], execution_semaphores[virtual_frame_index], submission_fences[virtual_frame_index]);

    auto present_result = api.present(swapchain, swapchain_image_index, execution_semaphores[virtual_frame_index]);
    handle_swapchain_result(present_result);

    virtual_frame_index = ++virtual_frame_index % virtual_frames_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(context.device, vkrenderer::virtual_frames_count, submission_fences, VK_TRUE, UINT64_MAX))

    auto swapchain_image_usages = api.get_image(swapchain.images[0]).usages;
    struct swapchain old_swapchain = swapchain;
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        swapchain_image_usages,
        old_swapchain.handle
    );

    api.destroy_swapchain(old_swapchain);

    swapchain_textures.resize(swapchain.image_count);
    for (size_t index = 0; index < swapchain.image_count; index++) {
        swapchain_textures[index] = new Texture();
        swapchain_textures[index]->device_image = swapchain.images[index];
    }
}

void vkrenderer::handle_swapchain_result(VkResult function_result) {
    switch(function_result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain();
        case VK_ERROR_SURFACE_LOST_KHR:
            assert(true && "surface lost");
        default:
            return;
    }
}

void vkrenderer::update_image(Texture* texture, void* data) {
    auto image = api.get_image(texture->device_image);
    auto max_texture_size = image.size.width * image.size.height * 4;
    auto buffer =  api.get_buffer(staging_buffer);
    if (buffer.size < max_texture_size) {
        api.destroy_buffer(staging_buffer);

        staging_buffer = api.create_buffer(max_texture_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    }

    std::memcpy(buffer.device_ptr, data, max_texture_size);

    VKRESULT(vkWaitForFences(context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(context.device, 1, &submission_fences[virtual_frame_index]))

    auto* cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, texture->device_image);

    api.copy_buffer(cmd_buf, staging_buffer, texture->device_image);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, texture->device_image);

    api.end_record(cmd_buf);


    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    VKRESULT(vkWaitForFences(context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
}

void vkrenderer::update_buffer(Buffer* buffer, void* data, off_t offset, size_t size) const {
    auto device_buffer = vkrenderer::api.get_buffer(buffer->device_buffer);

    size_t dynamic_offset = buffer->isStatic ? 0 : buffer->size * virtual_frame_index;

    std::memcpy((uint8_t*)device_buffer.device_ptr + dynamic_offset + offset, data, size);
}


Buffer* vkrenderer::create_buffer(size_t size, bool isStatic) {
    auto* buffer = new Buffer(size, isStatic);

    buffer->device_buffer = api.create_buffer(isStatic ? size : size * virtual_frames_count, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    return buffer;
}

Buffer* vkrenderer::create_index_buffer(size_t size, bool isStatic) {
    auto* buffer = new Buffer(size, isStatic);

    buffer->device_buffer = api.create_buffer(isStatic ? size : size * virtual_frames_count, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    return buffer;
}

Texture* vkrenderer::create_2d_texture(size_t width, size_t height, VkFormat format) {
     auto* texture = new Texture();
    texture->device_image = api.create_image(
        { .width = (uint32_t)width, .height = (uint32_t)height, .depth = 1 },
        format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    return texture;
}

void vkrenderer::destroy_2d_texture(Texture* texture) {
    api.destroy_image(texture->device_image);
    delete texture;
}

// TODO: Check if a similar sampler has been allocated
Sampler* vkrenderer::create_sampler(VkFilter filter, VkSamplerAddressMode address_mode) {
    auto* sampler = new Sampler();

    sampler->device_sampler = api.create_sampler(filter, address_mode);

    return sampler;
}

ComputeRenderpass* vkrenderer::create_compute_renderpass() {
    renderpasses.push_back(new ComputeRenderpass(api));

    return (ComputeRenderpass*)renderpasses.back();
}

PrimitiveRenderpass* vkrenderer::create_primitive_renderpass() {
    renderpasses.push_back(new PrimitiveRenderpass(api));

    return (PrimitiveRenderpass*)renderpasses.back();
}

Primitive* vkrenderer::create_primitive(PrimitiveRenderpass& primitive_render_pass) {
    return new Primitive(api, primitive_render_pass);
}

void vkrenderer::render() {
    // begin_frame();

    auto* cmd_buf = command_buffers[virtual_frame_index];

    for (auto& renderpass: renderpasses) {
        renderpass->execute(*this, cmd_buf);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, ((ComputeRenderpass*)renderpass)->output_texture->device_image);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, swapchain.images[swapchain_image_index]);

        api.blit_full(cmd_buf, ((ComputeRenderpass*)renderpass)->output_texture->device_image, swapchain.images[swapchain_image_index]);
    }


    // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, swapchain.images[swapchain_image_index]);

    // renderpasses[1]->execute(*this, cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, swapchain.images[swapchain_image_index]);

    // finish_frame();
}
