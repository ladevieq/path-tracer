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

    create_surface(wnd);
    create_pipeline();
    create_swapchain();
    create_command_buffers();
    create_descriptor_sets();
    create_fences();
    create_semaphores();

    compute_shader_buffer = api.create_buffer(sizeof(inputs), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    std::memcpy(compute_shader_buffer.mapped_ptr, &inputs, sizeof(inputs));

    update_descriptors_buffer();
    update_descriptors_image();
}

vkrenderer::~vkrenderer() {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))

    api.destroy_buffer(compute_shader_buffer);
    destroy_fences();
    destroy_semaphores();
    destroy_descriptor_sets();
    destroy_command_buffers();
    destroy_pipeline();
    destroy_swapchain();
    destroy_surface();
}

void vkrenderer::compute(uint32_t width, uint32_t height) {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))
    VKRESULT(vkResetFences(api.context.device, 1, &submission_fences[frame_index]))

    auto acquire_result = vkAcquireNextImageKHR(api.context.device, swapchain, UINT64_MAX, acquire_semaphores[frame_index], VK_NULL_HANDLE, &current_image_index);
    handle_swapchain_result(acquire_result);

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
    undefined_to_general_barrier.image                  = swapchain_images[current_image_index];
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

    vkCmdBindDescriptorSets(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &compute_shader_sets[current_image_index], 0, nullptr);

    vkCmdBindPipeline(command_buffers[frame_index], VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);

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
    general_to_present_barrier.image                    = swapchain_images[current_image_index];
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
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &current_image_index;
    present_info.pResults           = nullptr;

    auto present_result = vkQueuePresentKHR(api.context.queue, &present_info);
    handle_swapchain_result(present_result);

    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))

    frame_index = ++frame_index % swapchain_images_count;
}

void vkrenderer::recreate_swapchain() {
    VKRESULT(vkWaitForFences(api.context.device, 1, &submission_fences[frame_index], VK_TRUE, UINT64_MAX))

    destroy_swapchain_images();

    create_swapchain();

    update_descriptors_image();
}


void vkrenderer::create_surface(window& wnd) {
#if defined(LINUX)
    VkXcbSurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.connection                  = wnd.connection;
    create_info.window                      = wnd.win;

    VKRESULT(vkCreateXcbSurfaceKHR(api.context.instance, &create_info, nullptr, &platform_surface))
#elif defined(WINDOWS)
    VkWin32SurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.hinstance                   = GetModuleHandle(NULL);
    create_info.hwnd                        = wnd.win_handle;

    VKRESULT(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &platform_surface))
#endif
}

void vkrenderer::create_swapchain() {
    VkBool32 queue_support_presentation = VK_FALSE;
    VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(api.context.physical_device, api.context.queue_index, platform_surface, &queue_support_presentation))

    VkSurfaceCapabilitiesKHR caps;
    VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(api.context.physical_device, platform_surface, &caps))

    VkExtent2D wantedExtent = caps.currentExtent;

    if (wantedExtent.width < caps.minImageExtent.width) {
        wantedExtent.width = caps.minImageExtent.width;
    } else if (wantedExtent.width > caps.maxImageExtent.width) {
        wantedExtent.width = caps.maxImageExtent.width;
    }

    if (wantedExtent.height < caps.minImageExtent.height) {
        wantedExtent.height = caps.minImageExtent.height;
    } else if (wantedExtent.height > caps.maxImageExtent.height) {
        wantedExtent.height = caps.maxImageExtent.height;
    }

    uint32_t supported_formats_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(api.context.physical_device, platform_surface, &supported_formats_count,  VK_NULL_HANDLE))

    std::vector<VkSurfaceFormatKHR> supported_formats { supported_formats_count };
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(api.context.physical_device, platform_surface, &supported_formats_count,  supported_formats.data()))

    uint32_t supported_present_modes_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(api.context.physical_device, platform_surface, &supported_present_modes_count, VK_NULL_HANDLE))

    std::vector<VkPresentModeKHR> supported_present_modes { supported_present_modes_count };
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(api.context.physical_device, platform_surface, &supported_present_modes_count, supported_present_modes.data()))

    surface_format = supported_formats[0];

    VkSwapchainKHR old_swapchain            = swapchain;

    VkSwapchainCreateInfoKHR create_info    = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.surface                     = platform_surface;
    create_info.minImageCount               = swapchain_images_count;
    create_info.imageFormat                 = surface_format.format;
    create_info.imageColorSpace             = surface_format.colorSpace;
    create_info.imageExtent                 = wantedExtent;
    create_info.imageArrayLayers            = 1;
    create_info.imageUsage                  = VK_IMAGE_USAGE_STORAGE_BIT;
    create_info.imageSharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform                = caps.currentTransform;
    create_info.compositeAlpha              = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode                 = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped                     = VK_TRUE;
    create_info.oldSwapchain                = old_swapchain;

    VKRESULT(vkCreateSwapchainKHR(api.context.device, &create_info, nullptr, &swapchain))

    if (old_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(api.context.device, old_swapchain, nullptr);
    }

    swapchain_images_count = 0;
    VKRESULT(vkGetSwapchainImagesKHR(api.context.device, swapchain, (uint32_t*)&swapchain_images_count, VK_NULL_HANDLE))

    swapchain_images = std::vector<VkImage>{ swapchain_images_count };
    swapchain_images_views = std::vector<VkImageView>{ swapchain_images_count };
    VKRESULT(vkGetSwapchainImagesKHR(api.context.device, swapchain, (uint32_t*)&swapchain_images_count, swapchain_images.data()))

    for (size_t index = 0; index < swapchain_images_count; index++) {
        VkImageSubresourceRange image_subresource_range = {};
        image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel            = 0;
        image_subresource_range.levelCount              = 1;
        image_subresource_range.baseArrayLayer          = 0;
        image_subresource_range.layerCount              = 1;

        VkImageViewCreateInfo image_view_create_info    = {};
        image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext                    = nullptr;
        image_view_create_info.flags                    = 0;
        image_view_create_info.image                    = swapchain_images[index];
        image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                   = surface_format.format;
        image_view_create_info.components               = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_create_info.subresourceRange        = image_subresource_range;

        VKRESULT(vkCreateImageView(api.context.device, &image_view_create_info, nullptr, &swapchain_images_views[index]))
    }
}

void vkrenderer::create_pipeline() {
    std::vector<uint8_t> shader_code = get_shader_code("./shaders/compute.comp.spv");
    if (shader_code.size() == 0) {
        exit(1);
    }

    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = nullptr;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = shader_code.size();
    shader_create_info.pCode                        = (uint32_t*)shader_code.data();

    VKRESULT(vkCreateShaderModule(api.context.device, &shader_create_info, nullptr, &compute_shader_module))

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = nullptr;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = compute_shader_module;
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = nullptr;



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

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = nullptr;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = bindings.size();
    descriptor_set_layout_create_info.pBindings     = bindings.data();

    VKRESULT(vkCreateDescriptorSetLayout(api.context.device, &descriptor_set_layout_create_info, nullptr, &compute_shader_layout))

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &compute_shader_layout;
    layout_create_info.pushConstantRangeCount       = 0;
    layout_create_info.pPushConstantRanges          = nullptr;

    VKRESULT(vkCreatePipelineLayout(api.context.device, &layout_create_info, nullptr, &compute_pipeline_layout))

    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = nullptr;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = compute_pipeline_layout;

    VKRESULT(vkCreateComputePipelines(
        api.context.device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &compute_pipeline
    ))

    std::cerr << "Compute pipeline ready" << std::endl;
}

void vkrenderer::create_command_buffers() {
    VkCommandPoolCreateInfo cmd_pool_create_info    = {};
    cmd_pool_create_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext                      = nullptr;
    cmd_pool_create_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex           = api.context.queue_index;

    VKRESULT(vkCreateCommandPool(api.context.device, &cmd_pool_create_info, nullptr, &command_pool))

    VkCommandBufferAllocateInfo cmd_buf_allocate_info   = {};
    cmd_buf_allocate_info.sType                         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_allocate_info.pNext                         = nullptr;
    cmd_buf_allocate_info.commandPool                   = command_pool;
    cmd_buf_allocate_info.level                         = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_allocate_info.commandBufferCount            = swapchain_images_count;

    command_buffers = std::vector<VkCommandBuffer> { swapchain_images_count };
    vkAllocateCommandBuffers(api.context.device, &cmd_buf_allocate_info, command_buffers.data());
}
void vkrenderer::create_descriptor_sets() {
    std::vector<VkDescriptorPoolSize> descriptor_pools_sizes = {
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount                    = (uint32_t)swapchain_images_count,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount                    = (uint32_t)swapchain_images_count,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info  = {};
    descriptor_pool_create_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext                       = nullptr;
    descriptor_pool_create_info.flags                       = 0;
    descriptor_pool_create_info.maxSets                     = swapchain_images_count;
    descriptor_pool_create_info.poolSizeCount               = descriptor_pools_sizes.size();
    descriptor_pool_create_info.pPoolSizes                  = descriptor_pools_sizes.data();

    VKRESULT(vkCreateDescriptorPool(api.context.device, &descriptor_pool_create_info, nullptr, &descriptor_pool))

    std::vector<VkDescriptorSetLayout> sets_layouts = { swapchain_images_count, compute_shader_layout };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info    = {};
    descriptor_set_allocate_info.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext                          = nullptr;
    descriptor_set_allocate_info.descriptorPool                 = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount             = sets_layouts.size();
    descriptor_set_allocate_info.pSetLayouts                    = sets_layouts.data();

    compute_shader_sets = std::vector<VkDescriptorSet> { swapchain_images_count };
    vkAllocateDescriptorSets(api.context.device, &descriptor_set_allocate_info, compute_shader_sets.data());
}

void vkrenderer::create_fences() {
    for (size_t fence_index = 0; fence_index < swapchain_images_count; fence_index++) {
        VkFence submission_fence;
        VkFenceCreateInfo create_info   = {};
        create_info.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        create_info.pNext               = nullptr;
        create_info.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

        VKRESULT(vkCreateFence(api.context.device, &create_info, nullptr, &submission_fence))
        
        submission_fences.push_back(submission_fence);
    }
}

void vkrenderer::create_semaphores() {
    for (size_t semaphore_index = 0; semaphore_index < swapchain_images_count; semaphore_index++) {
        VkSemaphore execution_semaphore;
        VkSemaphore acquire_semaphore;
        VkSemaphoreCreateInfo create_info   = {};
        create_info.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        create_info.pNext                   = nullptr;
        create_info.flags                   = 0;

        VKRESULT(vkCreateSemaphore(api.context.device, &create_info, nullptr, &execution_semaphore))
        VKRESULT(vkCreateSemaphore(api.context.device, &create_info, nullptr, &acquire_semaphore))

        execution_semaphores.push_back(execution_semaphore);
        acquire_semaphores.push_back(acquire_semaphore);
    }
}

void vkrenderer::destroy_surface() {
    vkDestroySurfaceKHR(api.context.instance, platform_surface, nullptr);
}

void vkrenderer::destroy_swapchain() {
    vkDestroySwapchainKHR(api.context.device, swapchain, nullptr);

    destroy_swapchain_images();
}

void vkrenderer::destroy_swapchain_images() {
    for (auto view: swapchain_images_views) {
        vkDestroyImageView(api.context.device, view, nullptr);
    }

    swapchain_images_views.clear();
    swapchain_images.clear();
}

void vkrenderer::destroy_pipeline() {
    vkDestroyShaderModule(api.context.device, compute_shader_module, nullptr);
    vkDestroyDescriptorSetLayout(api.context.device, compute_shader_layout, nullptr);
    vkDestroyPipelineLayout(api.context.device, compute_pipeline_layout, nullptr);
    vkDestroyPipeline(api.context.device, compute_pipeline, nullptr);
}

void vkrenderer::destroy_command_buffers() {
    vkFreeCommandBuffers(api.context.device, command_pool, swapchain_images_count, command_buffers.data());

    vkDestroyCommandPool(api.context.device, command_pool, nullptr);
}

void vkrenderer::destroy_descriptor_sets() {
    // vkFreeDescriptorSets(device, descriptor_pool, swapchain_images_count, compute_shader_sets.data());

    vkDestroyDescriptorPool(api.context.device, descriptor_pool, nullptr);
}

void vkrenderer::destroy_fences() {
    for (auto fence: submission_fences) {
        vkDestroyFence(api.context.device, fence, nullptr);
    }

    submission_fences.clear();
}

void vkrenderer::destroy_semaphores() {
    for (auto semaphore: acquire_semaphores) {
        vkDestroySemaphore(api.context.device, semaphore, nullptr);
    }

    acquire_semaphores.clear();

    for (auto semaphore: execution_semaphores) {
        vkDestroySemaphore(api.context.device, semaphore, nullptr);
    }

    execution_semaphores.clear();
}

void vkrenderer::update_descriptors_buffer() {
    std::vector<VkDescriptorBufferInfo> descriptors_buf_info    { swapchain_images_count };
    std::vector<VkWriteDescriptorSet> write_descriptors         { swapchain_images_count };

    for (size_t set_index = 0; set_index < swapchain_images_count; set_index++) {
        // Update uniform buffer descriptor
        descriptors_buf_info[set_index].buffer                  = compute_shader_buffer.vkbuffer;
        descriptors_buf_info[set_index].offset                  = 0;
        descriptors_buf_info[set_index].range                   = VK_WHOLE_SIZE;

        write_descriptors[set_index].sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[set_index].pNext                  = nullptr;
        write_descriptors[set_index].dstSet                 = compute_shader_sets[set_index];
        write_descriptors[set_index].dstBinding             = 0;
        write_descriptors[set_index].dstArrayElement        = 0;
        write_descriptors[set_index].descriptorCount        = 1;
        write_descriptors[set_index].descriptorType         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write_descriptors[set_index].pImageInfo             = VK_NULL_HANDLE;
        write_descriptors[set_index].pBufferInfo            = &descriptors_buf_info[set_index];
        write_descriptors[set_index].pTexelBufferView       = VK_NULL_HANDLE;
    }

    vkUpdateDescriptorSets(api.context.device, write_descriptors.size(), write_descriptors.data(), 0, nullptr);
}

void vkrenderer::update_descriptors_image() {
    std::vector<VkDescriptorImageInfo> descriptors_image_info   { swapchain_images_count };
    std::vector<VkWriteDescriptorSet> write_descriptors         { swapchain_images_count };

    for (size_t set_index = 0; set_index < swapchain_images_count; set_index++) {
        // Update storage image descriptor
        descriptors_image_info[set_index].sampler       = VK_NULL_HANDLE;
        descriptors_image_info[set_index].imageView     = swapchain_images_views[set_index];
        descriptors_image_info[set_index].imageLayout   = VK_IMAGE_LAYOUT_GENERAL;

        write_descriptors[set_index].sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptors[set_index].pNext              = nullptr;
        write_descriptors[set_index].dstSet             = compute_shader_sets[set_index];
        write_descriptors[set_index].dstBinding         = 1;
        write_descriptors[set_index].dstArrayElement    = 0;
        write_descriptors[set_index].descriptorCount    = 1;
        write_descriptors[set_index].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        write_descriptors[set_index].pImageInfo         = &descriptors_image_info[set_index];
        write_descriptors[set_index].pBufferInfo        = VK_NULL_HANDLE;
        write_descriptors[set_index].pTexelBufferView   = VK_NULL_HANDLE;
    }

    vkUpdateDescriptorSets(api.context.device, write_descriptors.size(), write_descriptors.data(), 0, nullptr);
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
