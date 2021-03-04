#include <iostream>
#include <cstring>
#include <cassert>

#include "imgui.h"
#include "vk-renderer.hpp"
#include "thirdparty/vk_mem_alloc.h"

#include "defines.hpp"
#include "utils.hpp"
#include "window.hpp"

#define VKRESULT(result) assert(result == VK_SUCCESS);

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
    };
    ui_pipeline = api.create_graphics_pipeline("ui", ui_sets_bindings, (VkShaderStageFlagBits)(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT), render_pass);
    ui_sets = api.create_descriptor_sets(ui_pipeline.descriptor_set_layout, swapchain.image_count);

    ImGui::CreateContext();
    auto io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    uint8_t* pixels = nullptr;
    int atlas_width, atlas_height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &atlas_width, &atlas_height);

    VkExtent3D atlas_size = {
        .width = (uint32_t)atlas_width,
        .height = (uint32_t)atlas_height,
        .depth = 1,
    };
    ui_texture = api.create_image(atlas_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);
    io.Fonts->SetTexID((void*)&ui_texture);

    ui_vertex_buffers.resize(swapchain.image_count);
    ui_index_buffers.resize(swapchain.image_count);
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

    api.destroy_image(ui_texture);

    for (auto& framebuffer: framebuffers) {
        api.destroy_framebuffer(framebuffer);
    }

    for (auto& v_buffer_array: ui_vertex_buffers) {
        for (auto& vertex_buffer: v_buffer_array) {
            api.destroy_buffer(vertex_buffer);
        }
    }

    for (auto& i_buffer_array: ui_index_buffers) {
        for (auto& index_buffer: i_buffer_array) {
            api.destroy_buffer(index_buffer);
        }
    }

    api.destroy_command_buffers(command_buffers);
    api.destroy_pipeline(compute_pipeline);
    api.destroy_pipeline(ui_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::begin_frame() {

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize.x = (float)swapchain.extent.width;
    io.DisplaySize.y = (float)swapchain.extent.height;
    io.DisplayFramebufferScale = { 1.f, 1.f };

    ImGui::NewFrame();

    ImGui::Text("Hello, world!");

    ImGui::EndFrame();

    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();

    std::vector<Buffer> temp_vertex_buffers {};
    std::vector<Buffer> temp_index_buffers {};

    // Update descriptor with only the part of the buffer we are interested in
    for (size_t cmd_index = 0; cmd_index < draw_data->CmdListsCount; cmd_index++) {
        ImDrawList* draw_list = draw_data->CmdLists[cmd_index];
        auto vbuf = draw_list->VtxBuffer;
        auto ibuf = draw_list->IdxBuffer;

        // auto vk_vtx_buf = api.create_buffer(vertex_buffer.Size * sizeof(vertex_buffer.Data[0]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // std::memcpy(vk_vtx_buf.alloc_info.pMappedData, vertex_buffer.Data, vertex_buffer.Size * sizeof(vertex_buffer.Data[0]));

        // auto vk_idx_buf = api.create_buffer(index_buffer.Size * sizeof(index_buffer.Data[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        // std::memcpy(vk_idx_buf.alloc_info.pMappedData, index_buffer.Data, index_buffer.Size * sizeof(index_buffer.Data[0]));

        // temp_vertex_buffers.push_back(vk_vtx_buf);
        // temp_index_buffers.push_back(vk_idx_buf);

        vertex_buffer = api.create_buffer(vbuf.Size * sizeof(vbuf.Data[0]), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        std::memcpy(vertex_buffer.alloc_info.pMappedData, vbuf.Data, vbuf.Size * sizeof(vbuf.Data[0]));
        vmaFlushAllocation(api.context.allocator, vertex_buffer.alloc, 0, VK_WHOLE_SIZE);

        index_buffer = api.create_buffer(ibuf.Size * sizeof(ibuf.Data[0]), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        std::memcpy(index_buffer.alloc_info.pMappedData, ibuf.Data, ibuf.Size * sizeof(ibuf.Data[0]));
        vmaFlushAllocation(api.context.allocator, index_buffer.alloc, 0, VK_WHOLE_SIZE);
    }


    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    // for (auto& vertex_buffer: ui_vertex_buffers[frame_index]) {
    //     api.destroy_buffer(vertex_buffer);
    // }

    // for (auto& index_buffer: ui_index_buffers[frame_index]) {
    //     api.destroy_buffer(index_buffer);
    // }

    // ui_vertex_buffers[frame_index].clear();
    // ui_index_buffers[frame_index].clear();

    // ui_vertex_buffers[frame_index] = std::move(temp_vertex_buffers);
    // ui_index_buffers[frame_index] = std::move(temp_index_buffers);

    swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    auto cmd_buf = command_buffers[frame_index];

    auto output_image = accumulation_images[0];
    auto accumulation_image = accumulation_images[1];

    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], compute_sets_bindings[1], output_image.view);
    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], compute_sets_bindings[2], accumulation_image.view);

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


    // std::vector<VkBufferMemoryBarrier> buffers_barrier = {};
    // VkBufferMemoryBarrier vertex_buffer_barrier  = {};
    // vertex_buffer_barrier.sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    // vertex_buffer_barrier.pNext                  = VK_NULL_HANDLE;
    // vertex_buffer_barrier.srcAccessMask          = VK_ACCESS_MEMORY_WRITE_BIT;
    // vertex_buffer_barrier.dstAccessMask          = VK_ACCESS_MEMORY_WRITE_BIT;
    // vertex_buffer_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    // vertex_buffer_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    // vertex_buffer_barrier.buffer                 = vertex_buffer.handle;
    // vertex_buffer_barrier.offset                 = 0;
    // vertex_buffer_barrier.size                   = VK_WHOLE_SIZE;
    // buffers_barrier.push_back(vertex_buffer_barrier);

    // VkBufferMemoryBarrier index_buffer_barrier  = {};
    // index_buffer_barrier.sType                  = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    // index_buffer_barrier.pNext                  = VK_NULL_HANDLE;
    // index_buffer_barrier.srcAccessMask          = VK_ACCESS_MEMORY_WRITE_BIT;
    // index_buffer_barrier.dstAccessMask          = VK_ACCESS_MEMORY_WRITE_BIT;
    // index_buffer_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    // index_buffer_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    // index_buffer_barrier.buffer                 = index_buffer.handle;
    // index_buffer_barrier.offset                 = 0;
    // index_buffer_barrier.size                   = VK_WHOLE_SIZE;
    // buffers_barrier.push_back(index_buffer_barrier);

    // vkCmdPipelineBarrier(
    //     cmd_buf,
    //     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    //     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    //     0,
    //     0,
    //     nullptr,
    //     2,
    //     buffers_barrier.data(),
    //     0,
    //     nullptr
    // );

    // VkMemoryBarrier memory_barrier  = {};
    // memory_barrier.sType            = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    // memory_barrier.pNext            = VK_NULL_HANDLE;
    // memory_barrier.srcAccessMask    = VK_ACCESS_MEMORY_WRITE_BIT;
    // memory_barrier.dstAccessMask    = VK_ACCESS_INDEX_READ_BIT;

    // vkCmdPipelineBarrier(
    //     cmd_buf,
    //     VK_PIPELINE_STAGE_HOST_BIT,
    //     VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    //     0,
    //     1,
    //     &memory_barrier,
    //     0,
    //     nullptr,
    //     0,
    //     nullptr
    // );

    vkCmdPipelineBarrier(
        cmd_buf,
        VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr
    );

    vkCmdBeginRenderPass(cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ui_pipeline.layout, 0, 1, &ui_sets[frame_index], 0, nullptr);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, ui_pipeline.handle);

    // for (size_t cmd_index = 0; cmd_index < draw_data->CmdListsCount; cmd_index++) {
    //     ImDrawList* draw_list = draw_data->CmdLists[cmd_index];

        // api.update_descriptor_set_buffer(ui_sets[frame_index], ui_sets_bindings[0], ui_vertex_buffers[frame_index][cmd_index]);

        // vkCmdBindIndexBuffer(cmd_buf, ui_index_buffers[frame_index][cmd_index].handle, 0, VK_INDEX_TYPE_UINT16);

        api.update_descriptor_set_buffer(ui_sets[frame_index], ui_sets_bindings[0], vertex_buffer);
        // auto temp_buffer = { 0, 1, 2, 2, 1, 3 };
        // std::memcpy(ui_index_buffers[frame_index][cmd_index].alloc_info.pMappedData, &temp_buffer, sizeof(temp_buffer));
        vkCmdBindIndexBuffer(cmd_buf, index_buffer.handle, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd_buf, 50, 1, 0, 0, 0);

    //     for (auto& draw_cmd: draw_list->CmdBuffer) {
    //         vkCmdDrawIndexed(cmd_buf, draw_cmd.ElemCount, 1, draw_cmd.IdxOffset, draw_cmd.VtxOffset, 0);
    //     }
    // }

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

    // std::swap(accumulation_images[0], accumulation_images[1]);

    frame_index = ++frame_index % swapchain.image_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        std::optional<std::reference_wrapper<Swapchain>> { swapchain }
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
