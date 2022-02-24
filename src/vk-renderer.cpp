#include <iostream>
#include <cstring>
#include <cassert>

#include "compute-renderpass.hpp"
#include "imgui.h"
#include "vk-renderer.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "defines.hpp"
#include "window.hpp"

#ifdef _DEBUG
#define VKRESULT(result) assert(result == VK_SUCCESS);
#else
#define VKRESULT(result) result;
#endif

struct VkVertex {
    float x, y;
    float u, v;
    uint32_t color;
    uint32_t padding[3];
};

struct UiTransforms {
    float scale_x, scale_y;
    float translate_x, translate_y;
};

vkrenderer::vkrenderer(window& wnd) {
    platform_surface = api.create_surface(wnd);
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_NULL_HANDLE
    );

    tonemapping_pipeline = api.create_compute_pipeline("tonemapping");

    command_buffers = api.create_command_buffers(virtual_frames_count);

    submission_fences = api.create_fences(virtual_frames_count);
    execution_semaphores = api.create_semaphores(virtual_frames_count);
    acquire_semaphores = api.create_semaphores(virtual_frames_count);


    std::vector<VkFormat> attachments_format { swapchain.surface_format.format };
    render_pass = api.create_render_pass(attachments_format, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    for (auto& current_image : swapchain.images) {
        std::vector<image> attachments { current_image };
        framebuffers.push_back(api.create_framebuffer(render_pass, attachments, swapchain.extent));
    }


    std::vector<VkDynamicState> dynamic_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    ui_pipeline = api.create_graphics_pipeline("ui", (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), render_pass, dynamic_states);

    ImGui::CreateContext();
    auto io = ImGui::GetIO();

    uint8_t* pixels = nullptr;
    int atlas_width, atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);

    VkExtent3D atlas_size = {
        .width = (uint32_t)atlas_width,
        .height = (uint32_t)atlas_height,
        .depth = 1,
    };
    staging_buffer = api.create_buffer(4096 * 4096 * 4 * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    std::memcpy(staging_buffer.alloc_info.pMappedData, pixels, atlas_width * atlas_height * sizeof(uint32_t));

    // ui_texture = api.create_image(atlas_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    // io.Fonts->SetTexID((void*)&ui_texture);

    // VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    // Initialization frame
    // auto cmd_buf = command_buffers[virtual_frame_index];
    // api.start_record(cmd_buf);

    // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, ui_texture);

    // api.copy_buffer(cmd_buf, staging_buffer, ui_texture);

    // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, ui_texture);

    // api.end_record(cmd_buf);


    // api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    // VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))

    std::vector<image> images = {
        swapchain.images[0],
        swapchain.images[1],
        swapchain.images[2]
    };

    api.update_descriptor_images(images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    // std::vector<image> sampled_images = {
    //     ui_texture
    // };
    // api.update_descriptor_images(sampled_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

    // ui_texture_sampler = api.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    // std::vector<sampler> samplers = {
    //     ui_texture_sampler,
    // };
    // api.update_descriptor_samplers(samplers);

    virtual_frame_index++;

    ui_vertex_buffers.resize(virtual_frames_count);
    ui_index_buffers.resize(virtual_frames_count);
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    for (auto& renderpass : renderpasses) {
        delete renderpass;
    }

    api.destroy_fences(submission_fences);
    api.destroy_semaphores(execution_semaphores);
    api.destroy_semaphores(acquire_semaphores);

    api.destroy_render_pass(render_pass);

    // api.destroy_sampler(ui_texture_sampler);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }

    api.destroy_command_buffers(command_buffers);

    api.destroy_pipeline(tonemapping_pipeline);
    api.destroy_pipeline(ui_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::begin_frame() {
    ImGui::Render();
    // ImDrawData* draw_data = ImGui::GetDrawData();

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    // if (ui_vertex_buffers[virtual_frame_index].size < draw_data->TotalVtxCount * sizeof(VkVertex)) {
    //     api.destroy_buffer(ui_vertex_buffers[virtual_frame_index]);

    //     ui_vertex_buffers[virtual_frame_index] = api.create_buffer(draw_data->TotalVtxCount * sizeof(VkVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    // }
    // if (ui_index_buffers[virtual_frame_index].size < draw_data->TotalIdxCount * sizeof(ImDrawIdx)) {
    //     api.destroy_buffer(ui_index_buffers[virtual_frame_index]);

    //     ui_index_buffers[virtual_frame_index] = api.create_buffer(draw_data->TotalIdxCount * sizeof(ImDrawIdx), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    // }

    // off_t vertex_buffer_offset = 0;
    // off_t index_buffer_offset = 0;
    // for (uint32_t draw_list_index = 0; draw_list_index < draw_data->CmdListsCount; draw_list_index++) {
    //     auto vertex_buffer = draw_data->CmdLists[draw_list_index]->VtxBuffer;
    //     auto index_buffer = draw_data->CmdLists[draw_list_index]->IdxBuffer;
    //     std::vector<VkVertex> vertices;

    //     for (auto& imVertex: vertex_buffer) {
    //         vertices.push_back(*((VkVertex*)&imVertex));
    //     }

    //     std::memcpy((void*) ((uint64_t)ui_vertex_buffers[virtual_frame_index].alloc_info.pMappedData + (vertex_buffer_offset * sizeof(VkVertex))), vertices.data(), vertex_buffer.Size * sizeof(VkVertex));
    //     std::memcpy((void*) ((uint64_t)ui_index_buffers[virtual_frame_index].alloc_info.pMappedData + (index_buffer_offset * sizeof(ImDrawIdx))), index_buffer.Data, index_buffer.Size * sizeof(ImDrawIdx));

    //     vertex_buffer_offset += vertex_buffer.Size;
    //     index_buffer_offset += index_buffer.Size;
    // }

    swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[virtual_frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    api.start_record(command_buffers[virtual_frame_index]);

}

void vkrenderer::ui() {
    ImDrawData* draw_data = ImGui::GetDrawData();

    auto cmd_buf = command_buffers[virtual_frame_index];

    api.begin_render_pass(cmd_buf, render_pass, framebuffers[swapchain_image_index], swapchain.extent, ui_pipeline);

    VkViewport viewport = {};
    viewport.x          = draw_data->DisplayPos.x;
    viewport.y          = draw_data->DisplayPos.y;
    viewport.width      = draw_data->DisplaySize.x;
    viewport.height     = draw_data->DisplaySize.y;
    viewport.minDepth   = 0.f;
    viewport.maxDepth   = 1.f;

    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    float scale[] {
        2.f / draw_data->DisplaySize.x,
        2.f / draw_data->DisplaySize.y,
    };
    float translate[] {
        -1.f - draw_data->DisplayPos.x * (2.f / draw_data->DisplaySize.x),
        -1.f - draw_data->DisplayPos.y * (2.f / draw_data->DisplaySize.y),
    };
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 64, 8, (void*)&ui_vertex_buffers[virtual_frame_index].device_address);
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 72, 8, (void*)&scale);
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 80, 8, (void*)&translate);

    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 96, 4, (void*)&ui_texture.bindless_sampled_index);
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 100, 4, (void*)&ui_texture_sampler.bindless_index);

    off_t vertex_buffer_offset = 0;
    off_t index_buffer_offset = 0;
    for (size_t cmd_index = 0; cmd_index < draw_data->CmdListsCount; cmd_index++) {
        ImDrawList* draw_list = draw_data->CmdLists[cmd_index];

        for (auto& draw_cmd: draw_list->CmdBuffer) {
            VkRect2D rect   = {};
            rect.offset     = {
                (int32_t)draw_cmd.ClipRect.x,
                (int32_t)draw_cmd.ClipRect.y
            };
            rect.extent     = {
                (uint32_t)draw_cmd.ClipRect.z,
                (uint32_t)draw_cmd.ClipRect.w,
            };
            vkCmdSetScissor(cmd_buf, 0, 1, &rect);

            api.draw(cmd_buf, ui_pipeline, ui_index_buffers[virtual_frame_index], draw_cmd.ElemCount, index_buffer_offset + draw_cmd.IdxOffset, vertex_buffer_offset + draw_cmd.VtxOffset);
        }

        vertex_buffer_offset += draw_list->VtxBuffer.Size;
        index_buffer_offset += draw_list->IdxBuffer.Size;
    }

    api.end_render_pass(cmd_buf);
}

void vkrenderer::compute(uint32_t width, uint32_t height) {
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, (void*)&scene_buffer_addr);
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 56, 4, (void*)&swapchain.images[swapchain_image_index].bindless_storage_index);
    // vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 60, 4, (void*)&accumulation_images[0].bindless_storage_index);

    // api.run_compute_pipeline(cmd_buf, tonemapping_pipeline, (width / 8) + 1, (height / 8) + 1, 1);
}

void vkrenderer::finish_frame() {
    auto cmd_buf = command_buffers[virtual_frame_index];

    // api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_SHADER_WRITE_BIT, 0, swapchain.images[swapchain_image_index]);

    api.end_record(cmd_buf);

    api.submit(cmd_buf, acquire_semaphores[virtual_frame_index], execution_semaphores[virtual_frame_index], submission_fences[virtual_frame_index]);

    auto present_result = api.present(swapchain, swapchain_image_index, execution_semaphores[virtual_frame_index]);
    handle_swapchain_result(present_result);

    virtual_frame_index = ++virtual_frame_index % virtual_frames_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    struct swapchain old_swapchain = swapchain;
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        old_swapchain.handle
    );

    api.destroy_swapchain(old_swapchain);

    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };

    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    api.end_record(cmd_buf);

    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[0]))

    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[0]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[0], VK_TRUE, UINT64_MAX))

    std::vector<image> images = {
        swapchain.images[0],
        swapchain.images[1],
        swapchain.images[2]
    };

    api.update_descriptor_images(images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }
    framebuffers.clear();

    for (auto& current_image : swapchain.images) {
        std::vector<image> attachments { current_image };
        framebuffers.push_back(api.create_framebuffer(render_pass, attachments, swapchain.extent));
    }
}

void vkrenderer::handle_swapchain_result(VkResult function_result) {
    switch(function_result) {
        case VK_ERROR_OUT_OF_DATE_KHR:
            return recreate_swapchain();
        case VK_ERROR_SURFACE_LOST_KHR:
            std::cerr << "surface lost" << std::endl;
        default:
            return;
    }
}

void vkrenderer::update_image(Texture* texture, void* data, size_t size) {
    if (staging_buffer.size < size) {
        api.destroy_buffer(staging_buffer);

        staging_buffer = api.create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    }

    std::memcpy(staging_buffer.alloc_info.pMappedData, data, size);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, texture->device_image);

    api.copy_buffer(cmd_buf, staging_buffer, texture->device_image);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, texture->device_image);

    api.end_record(cmd_buf);


    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))

}

Buffer* vkrenderer::create_buffer(size_t size, bool isStatic) {
    auto buffer = new Buffer(this, size, isStatic);

    buffer->device_buffer = api.create_buffer(isStatic ? size : size * virtual_frames_count, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

    return buffer;
}

Texture* vkrenderer::create_2d_texture(size_t width, size_t height, VkFormat format) {
     auto texture = new Texture();
    texture->device_image = api.create_image(
        { .width = (uint32_t)width, .height = (uint32_t)height, .depth = 1 },
        format,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    );

    api.update_descriptor_image(texture->device_image, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    api.update_descriptor_image(texture->device_image, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

    return texture;
}

// TODO: Check if a similar sampler has been allocated
Sampler* vkrenderer::create_sampler(VkFilter filter, VkSamplerAddressMode address_mode) {
    auto sampler = new Sampler();

    sampler->device_sampler = api.create_sampler(filter, address_mode);

    api.update_descriptor_sampler(sampler->device_sampler);

    return sampler;
}

ComputeRenderpass* vkrenderer::create_compute_renderpass() {
    renderpasses.push_back(new ComputeRenderpass(api));

    return (ComputeRenderpass*)renderpasses.back();
}

void vkrenderer::render() {
    begin_frame();

    auto cmd_buf = command_buffers[virtual_frame_index];

    for (auto& renderpass: renderpasses) {
        renderpass->execute(cmd_buf);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT, ((ComputeRenderpass*)renderpass)->output_texture->device_image);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, swapchain.images[swapchain_image_index]);

        api.blit_full(cmd_buf, ((ComputeRenderpass*)renderpass)->output_texture->device_image, swapchain.images[swapchain_image_index]);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, swapchain.images[swapchain_image_index]);
    }

    finish_frame();
}
