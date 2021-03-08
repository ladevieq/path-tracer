#include <iostream>
#include <cstring>
#include <cassert>

#include <functional>

#include "imgui.h"
#include "vk-renderer.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "defines.hpp"
#include "utils.hpp"
#include "window.hpp"

#define VKRESULT(result) assert(result == VK_SUCCESS);

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

vkrenderer::vkrenderer(window& wnd, const input_data& inputs) {
    platform_surface = api.create_surface(wnd);
    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        std::nullopt
    );

    compute_sets_bindings = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        }
    };
    compute_pipeline = api.create_compute_pipeline("compute", compute_sets_bindings);
    compute_shader_sets = api.create_descriptor_sets(compute_pipeline.descriptor_set_layout, swapchain.image_count);

    command_buffers = api.create_command_buffers(swapchain.image_count);

    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };
    accumulation_images = api.create_images(image_size, swapchain.surface_format.format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2);

    submission_fences = api.create_fences(swapchain.image_count);
    execution_semaphores = api.create_semaphores(swapchain.image_count);
    acquire_semaphores = api.create_semaphores(swapchain.image_count);

    compute_shader_buffer = api.create_buffer(sizeof(inputs), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    std::memcpy(compute_shader_buffer.alloc_info.pMappedData, &inputs, sizeof(inputs));

    for (size_t index = 0; index < swapchain.image_count; index++) {
        api.update_descriptor_set_buffer(compute_shader_sets[index], compute_sets_bindings[0], compute_shader_buffer);
    }

    std::vector<VkFormat> attachments_format { swapchain.surface_format.format };
    render_pass = api.create_render_pass(attachments_format);

    for (auto& image : swapchain.images) {
        std::vector<Image> attachments { image };
        framebuffers.push_back(api.create_framebuffer(render_pass, attachments, swapchain.extent));
    }

    ui_sets_bindings = {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
    };

    std::vector<VkDynamicState> dynamic_states {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    ui_pipeline = api.create_graphics_pipeline("ui", ui_sets_bindings, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), render_pass, dynamic_states);
    ui_sets = api.create_descriptor_sets(ui_pipeline.descriptor_set_layout, swapchain.image_count);

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

    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    auto cmd_buf = command_buffers[frame_index];
    api.start_record(cmd_buf);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, ui_texture);

    VkImageSubresourceLayers image_subresource_layers   = {};
    image_subresource_layers.aspectMask                 = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_layers.mipLevel                   = 0;
    image_subresource_layers.baseArrayLayer             = 0;
    image_subresource_layers.layerCount                 = 1;

    VkBufferImageCopy buffer_to_image_copy  = {};
    buffer_to_image_copy.bufferOffset       = 0;
    buffer_to_image_copy.bufferRowLength    = 0;
    buffer_to_image_copy.bufferImageHeight  = 0;
    buffer_to_image_copy.imageSubresource   = image_subresource_layers;
    buffer_to_image_copy.imageOffset        = { 0, 0, 0 };
    buffer_to_image_copy.imageExtent        = atlas_size;

    vkCmdCopyBufferToImage(cmd_buf, staging_buffer.handle, ui_texture.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_to_image_copy);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, 0, ui_texture);

    api.end_record(cmd_buf);


    api.submit(cmd_buf, VK_NULL_HANDLE, VK_NULL_HANDLE, submission_fences[frame_index]);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))

    api.destroy_buffer(staging_buffer);

    frame_index++;

    VkSamplerCreateInfo sampler_create_info     = {};
    sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext                   = VK_NULL_HANDLE;
    sampler_create_info.flags                   = 0;
    sampler_create_info.magFilter               = VK_FILTER_LINEAR;
    sampler_create_info.minFilter               = VK_FILTER_LINEAR;
    sampler_create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_create_info.mipLodBias              = 0.f;
    sampler_create_info.anisotropyEnable        = VK_FALSE;
    sampler_create_info.compareEnable           = VK_FALSE;
    sampler_create_info.compareOp               = VK_COMPARE_OP_ALWAYS;
    sampler_create_info.minLod                  = 0.f;
    sampler_create_info.maxLod                  = 0.f;
    sampler_create_info.borderColor             = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_create_info.unnormalizedCoordinates = VK_FALSE;

    VKRESULT(vkCreateSampler(api.context.device, &sampler_create_info, VK_NULL_HANDLE, &ui_texture_sampler))

    for (size_t index = 0; index < swapchain.image_count; index++) {
        api.update_descriptor_set_image(ui_sets[index], ui_sets_bindings[2], ui_texture.view, ui_texture_sampler);
    }

    ui_vertex_buffers.resize(swapchain.image_count);
    ui_index_buffers.resize(swapchain.image_count);

    for (size_t index = 0; index < swapchain.image_count; index++) {
        ui_transforms_buffers.push_back(api.create_buffer(sizeof(UiTransforms), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU));
        api.update_descriptor_set_buffer(ui_sets[index], ui_sets_bindings[1], ui_transforms_buffers[index]);
    }
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    api.destroy_buffer(compute_shader_buffer);
    api.destroy_fences(submission_fences);
    api.destroy_semaphores(execution_semaphores);
    api.destroy_semaphores(acquire_semaphores);

    api.destroy_descriptor_sets(compute_shader_sets);
    api.destroy_images(accumulation_images);

    api.destroy_render_pass(render_pass);

    vkDestroySampler(api.context.device, ui_texture_sampler, VK_NULL_HANDLE);

    api.destroy_image(ui_texture);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }

    for (auto& transforms_buffer: ui_transforms_buffers) {
        api.destroy_buffer(transforms_buffer);
    }

    for (auto& vertex_buffer: ui_vertex_buffers) {
        api.destroy_buffer(vertex_buffer);
    }

    for (auto& index_buffer: ui_index_buffers) {
        api.destroy_buffer(index_buffer);
    }

    api.destroy_command_buffers(command_buffers);
    api.destroy_pipeline(compute_pipeline);
    api.destroy_pipeline(ui_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::begin_frame() {
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    ((UiTransforms*)ui_transforms_buffers[frame_index].alloc_info.pMappedData)->scale_x = 2.f / draw_data->DisplaySize.x;
    ((UiTransforms*)ui_transforms_buffers[frame_index].alloc_info.pMappedData)->scale_y = 2.f / draw_data->DisplaySize.y;
    ((UiTransforms*)ui_transforms_buffers[frame_index].alloc_info.pMappedData)->translate_x = -1.f - draw_data->DisplayPos.x * (2.f / draw_data->DisplaySize.x);
    ((UiTransforms*)ui_transforms_buffers[frame_index].alloc_info.pMappedData)->translate_y = -1.f - draw_data->DisplayPos.y * (2.f / draw_data->DisplaySize.y);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    // Update descriptor with only the part of the buffer we are interested in
    if (ui_vertex_buffers[frame_index].size < draw_data->TotalVtxCount * sizeof(VkVertex)) {
        api.destroy_buffer(ui_vertex_buffers[frame_index]);

        ui_vertex_buffers[frame_index] = api.create_buffer(draw_data->TotalVtxCount * sizeof(VkVertex), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        api.update_descriptor_set_buffer(ui_sets[frame_index], ui_sets_bindings[0], ui_vertex_buffers[frame_index]);
    }
    if (ui_index_buffers[frame_index].size < draw_data->TotalIdxCount * sizeof(ImDrawIdx)) {
        api.destroy_buffer(ui_index_buffers[frame_index]);

        ui_index_buffers[frame_index] = api.create_buffer(draw_data->TotalIdxCount * sizeof(ImDrawIdx), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
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

        std::memcpy((void*) ((uint64_t)ui_vertex_buffers[frame_index].alloc_info.pMappedData + (vertex_buffer_offset * sizeof(VkVertex))), vertices.data(), vertex_buffer.Size * sizeof(VkVertex));
        std::memcpy((void*) ((uint64_t)ui_index_buffers[frame_index].alloc_info.pMappedData + (index_buffer_offset * sizeof(ImDrawIdx))), index_buffer.Data, index_buffer.Size * sizeof(ImDrawIdx));

        vertex_buffer_offset += vertex_buffer.Size;
        index_buffer_offset += index_buffer.Size;
    }

    swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    auto cmd_buf = command_buffers[frame_index];

    auto output_image = accumulation_images[0];
    auto accumulation_image = accumulation_images[1];

    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], compute_sets_bindings[1], output_image.view, VK_NULL_HANDLE);
    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], compute_sets_bindings[2], accumulation_image.view, VK_NULL_HANDLE);

    api.start_record(cmd_buf);

}

void vkrenderer::ui() {
    ImDrawData* draw_data = ImGui::GetDrawData();

    auto cmd_buf = command_buffers[frame_index];

    VkRect2D render_area = {
        {
            0,
            0,
        },
        swapchain.extent
    };
    VkClearValue clear_value = { 0.f, 0.f, 0.f, 1.f };
    VkRenderPassBeginInfo render_pass_begin_info    = {};
    render_pass_begin_info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext                    = VK_NULL_HANDLE;
    render_pass_begin_info.renderPass               = render_pass;
    render_pass_begin_info.framebuffer              = framebuffers[swapchain_image_index];
    render_pass_begin_info.renderArea               = render_area;
    render_pass_begin_info.clearValueCount          = 1;
    render_pass_begin_info.pClearValues             = &clear_value;


    VkMemoryBarrier memory_barrier  = {};
    memory_barrier.sType            = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    memory_barrier.pNext            = VK_NULL_HANDLE;
    memory_barrier.srcAccessMask    = VK_ACCESS_MEMORY_WRITE_BIT;
    memory_barrier.dstAccessMask    = VK_ACCESS_INDEX_READ_BIT;


    vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ui_pipeline.handle);

    VkViewport viewport = {};
    viewport.x          = draw_data->DisplayPos.x;
    viewport.y          = draw_data->DisplayPos.y;
    viewport.width      = draw_data->DisplaySize.x;
    viewport.height     = draw_data->DisplaySize.y;
    viewport.minDepth   = 0.f;
    viewport.maxDepth   = 1.f;

    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    off_t vertex_buffer_offset = 0;
    off_t index_buffer_offset = 0;
    for (size_t cmd_index = 0; cmd_index < draw_data->CmdListsCount; cmd_index++) {
        ImDrawList* draw_list = draw_data->CmdLists[cmd_index];

        vkCmdBindIndexBuffer(cmd_buf, ui_index_buffers[frame_index].handle, 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ui_pipeline.layout, 0, 1, &ui_sets[frame_index], 0, VK_NULL_HANDLE);

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

            vkCmdDrawIndexed(cmd_buf, draw_cmd.ElemCount, 1, index_buffer_offset + draw_cmd.IdxOffset, vertex_buffer_offset + draw_cmd.VtxOffset, 0);
        }

        vertex_buffer_offset += draw_list->VtxBuffer.Size;
        index_buffer_offset += draw_list->IdxBuffer.Size;
    }

    vkCmdEndRenderPass(cmd_buf);
}

void vkrenderer::compute(uint32_t width, uint32_t height, bool clear) {
    auto cmd_buf = command_buffers[frame_index];

    auto output_image = accumulation_images[0];
    auto accumulation_image = accumulation_images[1];

    if (clear) {
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, accumulation_image);

        VkClearColorValue color = { 0.f, 0.f, 0.f, 1.f };
        vkCmdClearColorImage(cmd_buf, accumulation_image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &accumulation_image.subresource_range);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, accumulation_image);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT, output_image);
    } else {
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, accumulation_image);
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT, output_image);
    }

    api.run_compute_pipeline(cmd_buf, compute_pipeline, compute_shader_sets[swapchain_image_index], width / 8, height / 8, 1);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, output_image);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, swapchain.images[swapchain_image_index]);

    api.blit_full(cmd_buf, output_image, swapchain.images[swapchain_image_index]);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, swapchain.images[swapchain_image_index]);
}

void vkrenderer::finish_frame() {
    auto cmd_buf = command_buffers[frame_index];

    api.end_record(cmd_buf);

    api.submit(cmd_buf, acquire_semaphores[frame_index], execution_semaphores[frame_index], submission_fences[frame_index]);

    auto present_result = api.present(swapchain, swapchain_image_index, execution_semaphores[frame_index]);
    handle_swapchain_result(present_result);

    std::swap(accumulation_images[0], accumulation_images[1]);

    frame_index = ++frame_index % swapchain.image_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        std::optional<std::reference_wrapper<Swapchain>> { std::ref(swapchain) }
    );

    api.destroy_images(accumulation_images);
    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };
    accumulation_images = api.create_images(image_size, swapchain.surface_format.format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }
    framebuffers.clear();

    for (auto& image : swapchain.images) {
        std::vector<Image> attachments { image };
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
