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
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT,
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
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        }
    };

    compute_pipeline = api.create_compute_pipeline("./shaders/compute.comp.spv", bindings);

    command_buffers = api.create_command_buffers(swapchain.image_count);

    compute_shader_sets = api.create_descriptor_sets(compute_pipeline.descriptor_set_layout, swapchain.image_count);

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
    std::memcpy(compute_shader_buffer.mapped_ptr, &inputs, sizeof(inputs));

    for (size_t index = 0; index < swapchain.image_count; index++) {
        api.update_descriptor_set_buffer(compute_shader_sets[index], bindings[0], compute_shader_buffer);
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

    api.destroy_command_buffers(command_buffers);
    api.destroy_compute_pipeline(compute_pipeline);

    api.destroy_swapchain(swapchain);
    api.destroy_surface(platform_surface);
}


void vkrenderer::compute(uint32_t width, uint32_t height, bool clear) {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    uint32_t swapchain_image_index = 0;
    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain.handle, UINT64_MAX, acquire_semaphores[frame_index], VK_NULL_HANDLE, &swapchain_image_index);
    handle_swapchain_result(acquire_result);

    auto accumulation_image_index = frame_index % 2;
    auto output_image = accumulation_images[accumulation_image_index];
    auto accumulation_image = accumulation_images[(accumulation_image_index + 1) % 2];

    VkDescriptorSetLayoutBinding images[2] {
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        }, {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        }
    };
    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], images[0], output_image.view);
    api.update_descriptor_set_image(compute_shader_sets[swapchain_image_index], images[1], accumulation_image.view);

    auto cmd_buf = command_buffers[frame_index];

    api.start_record(cmd_buf);

    if (clear) {
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, accumulation_image);

        VkClearColorValue color = { 0.f, 0.f, 0.f, 1.f };
        vkCmdClearColorImage(cmd_buf, accumulation_image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &accumulation_image.subresource_range);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, accumulation_image);

        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT, output_image);
    } else {
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_READ_BIT, accumulation_image);
        api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, VK_ACCESS_SHADER_WRITE_BIT, output_image);
    }

    api.run_compute_pipeline(cmd_buf, compute_pipeline, compute_shader_sets[swapchain_image_index], width / 8, height / 8, 1);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, output_image);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, VK_ACCESS_TRANSFER_WRITE_BIT, swapchain.images[swapchain_image_index]);

    api.blit_full(cmd_buf, output_image, swapchain.images[swapchain_image_index]);

    api.image_barrier(cmd_buf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, swapchain.images[swapchain_image_index]);

    api.end_record(cmd_buf);

    api.submit(cmd_buf, acquire_semaphores[frame_index], execution_semaphores[frame_index], submission_fences[frame_index]);

    auto present_result = api.present(swapchain, swapchain_image_index, execution_semaphores[frame_index]);
    handle_swapchain_result(present_result);

    frame_index = ++frame_index % swapchain.image_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, submission_fences.size(), submission_fences.data(), VK_TRUE, UINT64_MAX))

    swapchain = api.create_swapchain(
        platform_surface,
        min_swapchain_image_count,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        std::optional<Swapchain> { swapchain }
    );

    api.destroy_images(accumulation_images);
    VkExtent3D image_size = {
        .width = swapchain.extent.width,
        .height = swapchain.extent.height,
        .depth = 1,
    };
    accumulation_images = api.create_images(image_size, swapchain.surface_format.format, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 2);
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
