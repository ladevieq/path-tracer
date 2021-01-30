#include <iostream>
#include <cstring>
#include <cassert>

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
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        std::nullopt
    );

    std::vector<VkDescriptorSetLayoutBinding> bindings {
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
        }
    };

    compute_pipeline = api.create_compute_pipeline("./shaders/compute.comp.spv", bindings);

    command_buffers = api.create_command_buffers(swapchain.image_count);

    compute_shader_sets = api.create_descriptor_sets(compute_pipeline.descriptor_set_layout, swapchain.image_count);

    submission_fences = api.create_fences(swapchain.image_count);
    execution_semaphores = api.create_semaphores(swapchain.image_count);
    acquire_semaphores = api.create_semaphores(swapchain.image_count);

    compute_shader_buffer = api.create_buffer(sizeof(inputs), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    std::memcpy(compute_shader_buffer.mapped_ptr, &inputs, sizeof(inputs));

    for (size_t index = 0; index < swapchain.image_count; index++) {
        api.update_descriptor_set_buffer(compute_shader_sets[index], bindings[0], compute_shader_buffer);
        api.update_descriptor_set_image(compute_shader_sets[index], bindings[1], swapchain.images[index].view);
    }
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    api.destroy_buffer(compute_shader_buffer);
    api.destroy_fences(submission_fences);
    api.destroy_semaphores(execution_semaphores);
    api.destroy_semaphores(acquire_semaphores);

    api.destroy_descriptor_sets(compute_shader_sets);

    api.destroy_command_buffers(command_buffers);
    api.destroy_compute_pipeline(compute_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}

void vkrenderer::compute(uint32_t width, uint32_t height) {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[frame_index], VK_NULL_HANDLE, &current_image_index);
    handle_swapchain_result(acquire_result);

    // API
    // api.start_record(cmd_buffer);
    // api.command_barrier();
    // bind pipeline
    // bind descriptor set
    // dispatch
    // api.command_barrier();
    // api.stop_record();
    // api.execute(cmd_buffer, wait_semaphore, signal_semaphore);
    // api.present(swapchain, img_index, wait_semaphore);

    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.pNext                    = nullptr;
    cmd_buf_begin_info.flags                    = 0;
    cmd_buf_begin_info.pInheritanceInfo         = nullptr;

    vkBeginCommandBuffer(command_buffers[frame_index], &cmd_buf_begin_info);
    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel            = 0;
    image_subresource_range.levelCount              = 1;
    image_subresource_range.baseArrayLayer          = 0;
    image_subresource_range.layerCount              = 1;

    VkImageMemoryBarrier undefined_to_general_barrier   = {};
    undefined_to_general_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    undefined_to_general_barrier.pNext                  = nullptr;
    undefined_to_general_barrier.srcAccessMask          = 0;
    undefined_to_general_barrier.dstAccessMask          = VK_ACCESS_SHADER_WRITE_BIT;
    undefined_to_general_barrier.oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED;
    undefined_to_general_barrier.newLayout              = VK_IMAGE_LAYOUT_GENERAL;
    undefined_to_general_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    undefined_to_general_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    undefined_to_general_barrier.image                  = swapchain.images[current_image_index].handle;
    undefined_to_general_barrier.subresourceRange       = image_subresource_range;

    vkCmdPipelineBarrier(
        command_buffers[frame_index],
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &undefined_to_general_barrier
    );

    vkCmdBindDescriptorSets(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.layout, 0, 1, &compute_shader_sets[current_image_index], 0, nullptr);

    vkCmdBindPipeline(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline.handle);

    vkCmdDispatch(command_buffers[frame_index], width / 8, height / 8, 1);

    VkImageMemoryBarrier general_to_present_barrier   = {};
    general_to_present_barrier.sType                    = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    general_to_present_barrier.pNext                    = nullptr;
    general_to_present_barrier.srcAccessMask            = VK_ACCESS_SHADER_WRITE_BIT;
    general_to_present_barrier.dstAccessMask            = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    general_to_present_barrier.oldLayout                = VK_IMAGE_LAYOUT_GENERAL;
    general_to_present_barrier.newLayout                = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    general_to_present_barrier.srcQueueFamilyIndex      = VK_QUEUE_FAMILY_IGNORED;
    general_to_present_barrier.dstQueueFamilyIndex      = VK_QUEUE_FAMILY_IGNORED;
    general_to_present_barrier.image                    = swapchain.images[current_image_index].handle;
    general_to_present_barrier.subresourceRange         = image_subresource_range;

    vkCmdPipelineBarrier(
        command_buffers[frame_index],
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &general_to_present_barrier
    );

    VKRESULT(vkEndCommandBuffer(command_buffers[frame_index]))

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info            = {};
    submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                   = nullptr;
    submit_info.waitSemaphoreCount      = 1;
    submit_info.pWaitSemaphores         = &acquire_semaphores[frame_index];
    submit_info.pWaitDstStageMask       = &wait_stage;
    submit_info.signalSemaphoreCount    = 1;
    submit_info.pSignalSemaphores       = &execution_semaphores[frame_index];
    submit_info.commandBufferCount      = 1;
    submit_info.pCommandBuffers         = &command_buffers[frame_index];

    VKRESULT(vkQueueSubmit(api.context.queue, 1, &submit_info, submission_fences[frame_index]))

    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = &execution_semaphores[frame_index];
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain.handle;
    present_info.pImageIndices      = &current_image_index;
    present_info.pResults           = nullptr;

    auto present_result = vkQueuePresentKHR(api.context.queue, &present_info);
    handle_swapchain_result(present_result);

    frame_index = ++frame_index % swapchain.image_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        std::optional<Swapchain> { swapchain }
    );

    VkDescriptorSetLayoutBinding binding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE,
    };

    for (size_t index = 0; index < swapchain.image_count; index++) {
        api.update_descriptor_set_image(compute_shader_sets[index], binding, swapchain.images[index].view);
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
