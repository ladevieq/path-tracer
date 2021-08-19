#include <iostream>
#include <cstring>
#include <cassert>

#include "imgui.h"
#include "vk-renderer.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "defines.hpp"
#include "scene.hpp"
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

vkrenderer::vkrenderer(window& wnd, size_t scene_buffer_size, size_t geometry_buffer_size, size_t bvh_buffer_size) {
    platform_surface = api.create_surface(wnd);
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_STORAGE_BIT,
        VK_NULL_HANDLE
    );

    compute_pipeline = api.create_compute_pipeline("compute");
    tonemapping_pipeline = api.create_compute_pipeline("tonemapping");

    command_buffers = api.create_command_buffers(virtual_frames_count);

    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };
    accumulation_images = api.create_images(image_size, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2);

    submission_fences = api.create_fences(virtual_frames_count);
    execution_semaphores = api.create_semaphores(virtual_frames_count);
    acquire_semaphores = api.create_semaphores(virtual_frames_count);

    scene_buffer = api.create_buffer(scene_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    geometry_buffer = api.create_buffer(geometry_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);
    bvh_buffer = api.create_buffer(bvh_buffer_size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);


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
    auto staging_buffer = api.create_buffer(atlas_width * atlas_height * sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    std::memcpy(staging_buffer.alloc_info.pMappedData, pixels, atlas_width * atlas_height * sizeof(uint32_t));

    ui_texture = api.create_image(atlas_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    io.Fonts->SetTexID((void*)&ui_texture);

    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    // Initialization frame
    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, accumulation_images[0]);
    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, accumulation_images[1]);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, ui_texture);

    api.copy_buffer(cmd_buf, staging_buffer, ui_texture);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, ui_texture);

    api.end_record(cmd_buf);


    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))

    api.destroy_buffer(staging_buffer);

    std::vector<image> images = {
        accumulation_images[0],
        accumulation_images[1],

        swapchain.images[0],
        swapchain.images[1],
        swapchain.images[2]
    };

    api.update_descriptor_images(images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

    std::vector<image> sampled_images = {
        ui_texture
    };
    api.update_descriptor_images(sampled_images, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);

    ui_texture_sampler = api.create_sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    std::vector<sampler> samplers = {
        ui_texture_sampler,
    };
    api.update_descriptor_samplers(samplers);

    virtual_frame_index++;

    ui_vertex_buffers.resize(virtual_frames_count);
    ui_index_buffers.resize(virtual_frames_count);
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    api.destroy_buffer(scene_buffer);
    api.destroy_buffer(geometry_buffer);
    api.destroy_buffer(bvh_buffer);
    api.destroy_fences(submission_fences);
    api.destroy_semaphores(execution_semaphores);
    api.destroy_semaphores(acquire_semaphores);

    api.destroy_images(accumulation_images);

    api.destroy_render_pass(render_pass);

    api.destroy_sampler(ui_texture_sampler);

    api.destroy_image(ui_texture);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }

    for (auto& vertex_buffer: ui_vertex_buffers) {
        api.destroy_buffer(vertex_buffer);
    }

    for (auto& index_buffer: ui_index_buffers) {
        api.destroy_buffer(index_buffer);
    }

    api.destroy_command_buffers(command_buffers);

    api.destroy_pipeline(compute_pipeline);
    api.destroy_pipeline(tonemapping_pipeline);
    api.destroy_pipeline(ui_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::begin_frame() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    if (ui_vertex_buffers[virtual_frame_index].size < draw_data->TotalVtxCount * sizeof(VkVertex)) {
        api.destroy_buffer(ui_vertex_buffers[virtual_frame_index]);

        ui_vertex_buffers[virtual_frame_index] = api.create_buffer(draw_data->TotalVtxCount * sizeof(VkVertex), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }
    if (ui_index_buffers[virtual_frame_index].size < draw_data->TotalIdxCount * sizeof(ImDrawIdx)) {
        api.destroy_buffer(ui_index_buffers[virtual_frame_index]);

        ui_index_buffers[virtual_frame_index] = api.create_buffer(draw_data->TotalIdxCount * sizeof(ImDrawIdx), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    }

    off_t vertex_buffer_offset = 0;
    off_t index_buffer_offset = 0;
    for (uint32_t draw_list_index = 0; draw_list_index < draw_data->CmdListsCount; draw_list_index++) {
        auto vertex_buffer = draw_data->CmdLists[draw_list_index]->VtxBuffer;
        auto index_buffer = draw_data->CmdLists[draw_list_index]->IdxBuffer;
        std::vector<VkVertex> vertices;

        for (auto& imVertex: vertex_buffer) {
            vertices.push_back(*((VkVertex*)&imVertex));
        }

        std::memcpy((void*) ((uint64_t)ui_vertex_buffers[virtual_frame_index].alloc_info.pMappedData + (vertex_buffer_offset * sizeof(VkVertex))), vertices.data(), vertex_buffer.Size * sizeof(VkVertex));
        std::memcpy((void*) ((uint64_t)ui_index_buffers[virtual_frame_index].alloc_info.pMappedData + (index_buffer_offset * sizeof(ImDrawIdx))), index_buffer.Data, index_buffer.Size * sizeof(ImDrawIdx));

        vertex_buffer_offset += vertex_buffer.Size;
        index_buffer_offset += index_buffer.Size;
    }

    swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[virtual_frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    api.start_record(command_buffers[virtual_frame_index]);

}

void vkrenderer::reset_accumulation() {
    auto cmd_buf = command_buffers[virtual_frame_index];
    VkClearColorValue clear_color = { 0.f, 0.f, 0.f, 1.f };

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, accumulation_images[0]);

    vkCmdClearColorImage(cmd_buf, accumulation_images[0].handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &accumulation_images[0].subresource_range);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, accumulation_images[0]);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, accumulation_images[1]);

    vkCmdClearColorImage(cmd_buf, accumulation_images[1].handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &accumulation_images[1].subresource_range);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, accumulation_images[1]);
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
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 32, 8, (void*)&ui_vertex_buffers[virtual_frame_index].device_address);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 40, 8, (void*)&scale);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 48, 8, (void*)&translate);

    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 56, 4, (void*)&ui_texture.bindless_sampled_index);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_FRAGMENT_BIT, 60, 4, (void*)&ui_texture_sampler.bindless_index);

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
    auto cmd_buf = command_buffers[virtual_frame_index];

    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, (void*)&scene_buffer.device_address);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 8, 8, (void*)&geometry_buffer.device_address);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 16, 8, (void*)&bvh_buffer.device_address);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 24, 4, (void*)&accumulation_images[0].bindless_storage_index);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 28, 4, (void*)&accumulation_images[1].bindless_storage_index);
    api.run_compute_pipeline(cmd_buf, compute_pipeline, (width / 8) + 1, (height / 8) + 1, 1);


    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, swapchain.images[swapchain_image_index]);

    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 8, (void*)&scene_buffer.device_address);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 8, 4, (void*)&swapchain.images[swapchain_image_index].bindless_storage_index);
    vkCmdPushConstants(cmd_buf, api.bindless_descriptor.pipeline_layout, VK_SHADER_STAGE_COMPUTE_BIT, 12, 4, (void*)&accumulation_images[0].bindless_storage_index);

    api.run_compute_pipeline(cmd_buf, tonemapping_pipeline, (width / 8) + 1, (height / 8) + 1, 1);
}

void vkrenderer::finish_frame() {
    auto cmd_buf = command_buffers[virtual_frame_index];

    api.end_record(cmd_buf);

    api.submit(cmd_buf, acquire_semaphores[virtual_frame_index], execution_semaphores[virtual_frame_index], submission_fences[virtual_frame_index]);

    auto present_result = api.present(swapchain, swapchain_image_index, execution_semaphores[virtual_frame_index]);
    handle_swapchain_result(present_result);

    std::swap(accumulation_images[0], accumulation_images[1]);

    virtual_frame_index = ++virtual_frame_index % virtual_frames_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    Swapchain old_swapchain = swapchain;
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        old_swapchain.handle
    );

    api.destroy_swapchain(old_swapchain);

    api.destroy_images(accumulation_images);
    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };

    accumulation_images = api.create_images(image_size, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2);

    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, accumulation_images[0]);
    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, accumulation_images[1]);

    api.end_record(cmd_buf);

    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[0]))

    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[0]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[0], VK_TRUE, UINT64_MAX))

    std::vector<image> images = {
        accumulation_images[0],
        accumulation_images[1],

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


// TODO: Integrate into the API so that we no longer have to wait for the fence
void vkrenderer::update_geometry_buffer(void* data) {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    auto staging_buffer = api.create_buffer(geometry_buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    std::memcpy(staging_buffer.alloc_info.pMappedData, data, staging_buffer.size);

    api.copy_buffer(cmd_buf, staging_buffer, geometry_buffer, staging_buffer.size);

    api.end_record(cmd_buf);

    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))

    api.destroy_buffer(staging_buffer);
}

void vkrenderer::update_bvh_buffer(void* data) {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[virtual_frame_index]))

    auto cmd_buf = command_buffers[virtual_frame_index];
    api.start_record(cmd_buf);

    auto staging_buffer = api.create_buffer(bvh_buffer.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    std::memcpy(staging_buffer.alloc_info.pMappedData, data, staging_buffer.size);

    api.copy_buffer(cmd_buf, staging_buffer, bvh_buffer, staging_buffer.size);

    api.end_record(cmd_buf);

    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[virtual_frame_index]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[virtual_frame_index], VK_TRUE, UINT64_MAX))

    api.destroy_buffer(staging_buffer);
}
