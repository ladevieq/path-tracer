#include "vk-device.hpp"

#include <cassert>
#include <vector>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vk-utils.hpp"
#include "vulkan-loader.hpp"
#include "window.hpp"

vkdevice* vkdevice::render_device = nullptr;

vkdevice::vkdevice(const window& window)
        : queues{
              {
                  .usages = VK_QUEUE_GRAPHICS_BIT |
                            VK_QUEUE_COMPUTE_BIT |
                            VK_QUEUE_TRANSFER_BIT,
              },
              {
                  .usages = VK_QUEUE_COMPUTE_BIT,
              },
              {
                  .usages = VK_QUEUE_TRANSFER_BIT,
              },
          } {
    load_vulkan();

    create_instance();

    create_surface(window);

#ifdef _DEBUG
    create_debug_layer_callback();
#endif // _DEBUG

    create_device();

    create_memory_allocator();

    create_swapchain();

    bindless = bindless_model::create_bindless_model();
}

vkdevice* vkdevice::init_device(const window& window) {
    vkdevice::render_device = static_cast<vkdevice*>(malloc(sizeof(vkdevice)));
    vkdevice::render_device = new (vkdevice::render_device) vkdevice(window);
    return render_device;
}

void vkdevice::free_device() {
    delete render_device;
    render_device = nullptr;
}

vkdevice::~vkdevice() {
    vkDeviceWaitIdle(device);

    bindless_model::destroy_bindless_model(bindless);

    for (auto& texture : textures) {
        destroy_texture(texture);
    }

    for (auto& buffer : buffers) {
        destroy_buffer(buffer);
    }

    for (auto& pipeline : pipelines) {
        destroy_pipeline(pipeline);
    }

    if (swapchain != nullptr) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }

    vmaDestroyAllocator(gpu_allocator);

    for (auto& queue : queues) {
        vkDestroyCommandPool(device, queue.command_pool, nullptr);
    }

    vkDestroyDevice(device, nullptr);

#ifdef _DEBUG
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
#endif // _DEBUG

    if (surface != nullptr) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}

handle<device_texture> vkdevice::create_texture(const texture_desc& desc) {
    device_texture device_texture{
        .desc = desc,
    };

    assert(desc.mips <= texture_desc::max_mips);

    VkImageCreateInfo create_info{
        .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext     = nullptr,
        .flags     = 0U,
        .imageType = desc.type,
        .format    = desc.format,
        .extent    = {
               .width  = desc.width,
               .height = desc.height,
               .depth  = desc.depth },
        .mipLevels             = desc.mips,
        .arrayLayers           = 1,
        .samples               = VK_SAMPLE_COUNT_1_BIT,
        .tiling                = VK_IMAGE_TILING_OPTIMAL,
        .usage                 = desc.usages,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VmaAllocationCreateInfo alloc_create_info{
        .flags          = 0U,
        .usage          = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags  = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .preferredFlags = 0U,
        .memoryTypeBits = 0U,
        .pool           = nullptr,
        .pUserData      = nullptr,
    };

    VKCHECK(vmaCreateImage(gpu_allocator, &create_info, &alloc_create_info, &device_texture.vk_image, &device_texture.alloc, nullptr));

    create_views(device_texture);

    // TODO: Handle mips views
    VkDescriptorImageInfo images_info[]{
        {
            nullptr,
            device_texture.whole_view,
            VK_IMAGE_LAYOUT_GENERAL,
        },
    };

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    if ((desc.usages & VK_IMAGE_USAGE_STORAGE_BIT) != 0) {
        device_texture.storage_index = storage_indices.add(&device_texture);

        write_descriptor_sets.push_back({
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = bindless.sets[BindlessSetType::GLOBAL],
            .dstBinding       = 2U,
            .dstArrayElement  = device_texture.storage_index,
            .descriptorCount  = sizeof(images_info) / sizeof(VkDescriptorImageInfo),
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo       = images_info,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr,
        });
    }
    if ((desc.usages & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) {
        device_texture.sampled_index = sampled_indices.add(&device_texture);

        write_descriptor_sets.push_back({
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = bindless.sets[BindlessSetType::GLOBAL],
            .dstBinding       = 1U,
            .dstArrayElement  = device_texture.sampled_index,
            .descriptorCount  = sizeof(images_info) / sizeof(VkDescriptorImageInfo),
            .descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo       = images_info,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr,
        });
    }

    vkUpdateDescriptorSets(device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0U, nullptr);

    return { textures.add(device_texture) };
}

handle<device_buffer> vkdevice::create_buffer(const buffer_desc& desc) {
    device_buffer device_buffer{
        .desc = desc,
    };

    VkBufferCreateInfo create_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0U,
        .size                  = desc.size,
        .usage                 = desc.usages,
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
    };

    VmaAllocatorCreateFlags alloc_create_flags =
        desc.memory_usage != VMA_MEMORY_USAGE_GPU_ONLY ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0U;

    VmaAllocationCreateInfo alloc_create_info{
        .flags          = alloc_create_flags,
        .usage          = static_cast<VmaMemoryUsage>(desc.memory_usage),
        .requiredFlags  = desc.memory_properties,
        .preferredFlags = 0U,
        .memoryTypeBits = 0U,
        .pool           = nullptr,
        .pUserData      = nullptr,
    };

    VmaAllocationInfo alloc_info{};

    VKCHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.vk_buffer, &device_buffer.alloc, &alloc_info));

    if ((desc.usages & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0U) {
        VkBufferDeviceAddressInfo address_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = device_buffer.vk_buffer,
        };
        device_buffer.device_address = vkGetBufferDeviceAddress(device, &address_info);
    }

    device_buffer.mapped_ptr = alloc_info.pMappedData;

    return { buffers.add(device_buffer) };
}

handle<device_pipeline> vkdevice::create_pipeline(const pipeline_desc& desc) {
    device_pipeline device_pipeline{
        .desc     = desc,
    };
    if (!desc.cs_code.empty()) {
        VkShaderModule shader_module;
        {
            const auto&              code = desc.cs_code;
            VkShaderModuleCreateInfo create_info{
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0U,
                .codeSize = code.size(),
                .pCode    = (uint32_t*)code.data(),
            };

            VKCHECK(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
        }

        VkPipelineShaderStageCreateInfo stage_create_info{
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0U,
            .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
            .module              = shader_module,
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        };

        VkComputePipelineCreateInfo create_info{
            .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .pNext              = nullptr,
            .flags              = 0U,
            .stage              = stage_create_info,
            .layout             = bindless.layout,
            .basePipelineHandle = nullptr,
            .basePipelineIndex  = 0,
        };

        device_pipeline.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
        VKCHECK(vkCreateComputePipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));

        vkDestroyShaderModule(device, shader_module, nullptr);
    } else {
        VkPipelineShaderStageCreateInfo shader_stages_create_info[2U]{
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0U,
                .stage               = VK_SHADER_STAGE_VERTEX_BIT,
                .pName               = "main",
                .pSpecializationInfo = nullptr,
            },
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0U,
                .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
                .pName               = "main",
                .pSpecializationInfo = nullptr,
            },
        };

        {
            const auto&              code = desc.vs_code;
            VkShaderModuleCreateInfo module_create_info{
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0U,
                .codeSize = code.size(),
                .pCode    = (uint32_t*)code.data(),
            };

            VKCHECK(vkCreateShaderModule(device, &module_create_info, nullptr, &shader_stages_create_info[0U].module));
        }
        {
            const auto&              code = desc.fs_code;
            VkShaderModuleCreateInfo module_create_info{
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0U,
                .codeSize = code.size(),
                .pCode    = (uint32_t*)code.data(),
            };

            VKCHECK(vkCreateShaderModule(device, &module_create_info, nullptr, &shader_stages_create_info[1U].module));
        }

        VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        };

        VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .pNext                  = nullptr,
            .flags                  = 0U,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };

        VkPipelineViewportStateCreateInfo viewport_state_create_info{
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = 0U,
            .viewportCount = 1U,
            .pViewports    = nullptr,
            .scissorCount  = 1U,
            .pScissors     = nullptr,
        };

        VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext                   = nullptr,
            .flags                   = 0U,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .cullMode                = VK_CULL_MODE_NONE,
            .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
            .lineWidth               = 1.f,
        };

        VkPipelineMultisampleStateCreateInfo multisample_state_create_info{
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .pNext                = nullptr,
            .flags                = 0,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable  = VK_FALSE,
            .minSampleShading     = 1.f,
        };

        VkPipelineColorBlendAttachmentState color_attachment_state{
            .blendEnable         = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp        = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .alphaBlendOp        = VK_BLEND_OP_ADD,
            .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext           = nullptr,
            .flags           = 0,
            .logicOpEnable   = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments    = &color_attachment_state,
        };

        VkDynamicState dynamic_states[]{
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext             = nullptr,
            .flags             = 0U,
            .dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState),
            .pDynamicStates    = dynamic_states,
        };

        VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
            .pNext                   = nullptr,
            .viewMask                = 0U,
            .colorAttachmentCount    = static_cast<uint32_t>(desc.color_attachments_format.size()),
            .pColorAttachmentFormats = desc.color_attachments_format.data(),
            .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
        };

        VkGraphicsPipelineCreateInfo create_info{
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext               = &pipeline_rendering_create_info,
            .flags               = 0U,
            .stageCount          = sizeof(shader_stages_create_info) / sizeof(VkPipelineShaderStageCreateInfo),
            .pStages             = shader_stages_create_info,
            .pVertexInputState   = &vertex_input_state_create_info,
            .pInputAssemblyState = &input_assembly_create_info,
            .pTessellationState  = nullptr,
            .pViewportState      = &viewport_state_create_info,
            .pRasterizationState = &rasterization_state_create_info,
            .pMultisampleState   = &multisample_state_create_info,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &color_blend_state_create_info,
            .pDynamicState       = &dynamic_state_create_info,
            .layout              = bindless.layout,
            .renderPass          = nullptr,
            .subpass             = 0U,
            .basePipelineHandle  = nullptr,
            .basePipelineIndex   = 0,
        };

        device_pipeline.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
        VKCHECK(vkCreateGraphicsPipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));
    }

    return { pipelines.add(device_pipeline) };
}

vksemaphore vkdevice::create_semaphore() {
    vksemaphore semaphore{
        .value = 0,
    };
    VkSemaphoreTypeCreateInfo semaphore_type_create_info{
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
        .initialValue  = 0U,
    };

    VkSemaphoreCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphore_type_create_info,
        .flags = 0U,
    };

    VKCHECK(vkCreateSemaphore(device, &create_info, nullptr, &semaphore.vk_semaphore));

    return semaphore;
}

void vkdevice::destroy_texture(const device_texture& texture) {
    destroy_texture({ .id = textures.find(texture) });
}

void vkdevice::destroy_texture(handle<device_texture> handle) {
    const auto& texture = get_texture(handle);
    textures.remove(handle.id);

    vmaDestroyImage(gpu_allocator, texture.vk_image, texture.alloc);

    vkDestroyImageView(device, texture.whole_view, nullptr);

    for (const auto& mip_view : texture.mips_views) {
        vkDestroyImageView(device, mip_view, nullptr);
    }
}

void vkdevice::destroy_buffer(const device_buffer& buffer) {
    destroy_buffer({ .id = buffers.find(buffer) });
}

void vkdevice::destroy_buffer(handle<device_buffer> handle) {
    const auto& buffer = get_buffer(handle);

    vmaDestroyBuffer(gpu_allocator, buffer.vk_buffer, buffer.alloc);
}

void vkdevice::destroy_pipeline(const device_pipeline& pipeline) {
    destroy_pipeline({ .id = pipelines.find(pipeline) });
}

void vkdevice::destroy_pipeline(handle<device_pipeline> handle) {
    const auto& pipeline = vkdevice::get_pipeline(handle);

    vkDestroyPipeline(device, pipeline.vk_pipeline, nullptr);
}

void vkdevice::wait(vksemaphore& semaphore) {
    VkSemaphoreWaitInfo wait_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .semaphoreCount = 1U,
        .pSemaphores = &semaphore.vk_semaphore,
        .pValues = &semaphore.value,
    };
    VKCHECK(vkWaitSemaphores(device, &wait_info, -1));
}

void vkdevice::submit(std::span<command_buffer> buffers, vksemaphore* wait, vksemaphore* signal) {
#define USE_SUBMIT2
    assert((max_submitable_command_buffers - buffers.size()) >= 0);
    const auto queue_type = buffers[0].queue_type;
#ifndef USE_SUBMIT2
    VkCommandBuffer to_submit[max_submitable_command_buffers];

    for (auto index{ 0U }; index < count; index++) {
        assert(queue_type == buffers[index].queue_type);
        to_submit[index] = buffers[index].vk_command_buffer;
    }

    VkSubmitInfo submit_info{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 0U,
        .pWaitSemaphores      = nullptr,
        .pWaitDstStageMask    = nullptr,
        .commandBufferCount   = static_cast<uint32_t>(count),
        .pCommandBuffers      = to_submit,
        .signalSemaphoreCount = 0U,
        .pSignalSemaphores    = nullptr,
    };

    VKCHECK(vkQueueSubmit(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));
#else
    VkCommandBufferSubmitInfo command_buffers_info[buffers.size()];

    for (auto index{ 0U }; index < buffers.size(); index++) {
        assert(queue_type == buffers[index].queue_type);

        auto& command_buffer_info         = command_buffers_info[index];
        command_buffer_info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.pNext         = nullptr;
        command_buffer_info.commandBuffer = buffers[index].vk_command_buffer;
        command_buffer_info.deviceMask    = 0U;
    }

    VkSemaphoreSubmitInfo wait_semaphore_info{
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .stageMask   = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        .deviceIndex = 0U,
    };

    VkSemaphoreSubmitInfo signal_semaphore_info{
        .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .pNext       = nullptr,
        .stageMask   = VK_PIPELINE_STAGE_2_NONE,
        .deviceIndex = 0U,
    };

    VkSubmitInfo2 submit_info{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0U,
        .commandBufferInfoCount   = static_cast<uint32_t>(buffers.size()),
        .pCommandBufferInfos      = command_buffers_info,
    };

    if (wait != nullptr) {
        wait_semaphore_info.semaphore      = wait->vk_semaphore;
        wait_semaphore_info.value          = wait->value;
        submit_info.waitSemaphoreInfoCount = 1U;
        submit_info.pWaitSemaphoreInfos    = &wait_semaphore_info;
    }

    if (signal != nullptr) {
        signal_semaphore_info.semaphore      = signal->vk_semaphore;
        signal_semaphore_info.value          = ++signal->value;

        submit_info.signalSemaphoreInfoCount = 1U;
        submit_info.pSignalSemaphoreInfos    = &signal_semaphore_info;
    }

    VKCHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));
#endif
}

void vkdevice::present(device_surface& surface, const vksemaphore& semaphore) {
    VkResult result;
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &semaphore.vk_semaphore,
        .swapchainCount = 1U,
        .pSwapchains = &surface.vk_swapchain,
        .pImageIndices = &surface.image_index,
        .pResults = &result,
    };

    VKCHECK(vkQueuePresentKHR(queues[static_cast<uint32_t>(QueueType::GRAPHICS)].vk_queue, &present_info));

    VKCHECK(result);
}

// private
void vkdevice::allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type) {
    assert((max_allocable_command_buffers - count) >= 0);
    const auto&                 queue = queues[static_cast<uint32_t>(type)];
    VkCommandBuffer             command_buffers[max_allocable_command_buffers];

    VkCommandBufferAllocateInfo allocate_info{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = queue.command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(count),
    };

    VKCHECK(vkAllocateCommandBuffers(device, &allocate_info, command_buffers));

    for (auto index{ 0U }; index < count; index++) {
        buffers[index].vk_command_buffer = command_buffers[index];
        buffers[index].queue_type        = type;
    }
}

void vkdevice::create_views(device_texture& texture) {
    VkImageAspectFlags    aspect_mask = ((texture.desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
                                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                                            : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageViewCreateInfo create_info{
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext      = nullptr,
        .flags      = 0U,
        .image      = texture.vk_image,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = texture.desc.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask     = aspect_mask,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    for (auto mip_index{ 0 }; mip_index < texture.desc.mips; mip_index++) {
        create_info.subresourceRange.baseMipLevel = mip_index;
        VKCHECK(vkCreateImageView(device, &create_info, nullptr, &texture.mips_views[mip_index]));
    }

    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount   = texture.desc.mips;
    VKCHECK(vkCreateImageView(device, &create_info, nullptr, &texture.whole_view));
}

// Presentation stuff
void vkdevice::create_surface(const window& window) {
    VkWin32SurfaceCreateInfoKHR create_info{
        .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext     = nullptr,
        .flags     = 0U,
        .hinstance = GetModuleHandle(nullptr),
        .hwnd      = window.handle,
    };

    VKCHECK(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface));
}

void vkdevice::create_swapchain() {
    VkBool32 supported = VK_FALSE;
    auto&    queue     = queues[static_cast<uint32_t>(QueueType::GRAPHICS)];
    VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue.index, surface, &supported));

    assert(supported);

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities));

    VkExtent2D extent = surface_capabilities.currentExtent;

    if (extent.width < surface_capabilities.minImageExtent.width) {
        extent.width = surface_capabilities.minImageExtent.width;
    } else if (extent.width > surface_capabilities.maxImageExtent.width) {
        extent.width = surface_capabilities.maxImageExtent.width;
    }

    if (extent.height < surface_capabilities.minImageExtent.height) {
        extent.height = surface_capabilities.minImageExtent.height;
    } else if (extent.height > surface_capabilities.maxImageExtent.height) {
        extent.height = surface_capabilities.maxImageExtent.height;
    }

    auto supported_formats_count = 0U;
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_formats_count, nullptr));

    std::vector<VkSurfaceFormatKHR> supported_formats{ supported_formats_count };
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_formats_count, supported_formats.data()));


    auto supported_present_modes_count = 0U;
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_present_modes_count, nullptr));

    std::vector<VkPresentModeKHR> supported_present_modes{ supported_present_modes_count };

    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_present_modes_count, supported_present_modes.data()));

    auto                     min_image_count = (surface_capabilities.minImageCount <= virtual_frames_count) ? virtual_frames_count : surface_capabilities.minImageCount;

    VkSwapchainCreateInfoKHR create_info     = {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                 = nullptr,
            .flags                 = 0U,
            .surface               = surface,
            .minImageCount         = min_image_count,
            .imageFormat           = supported_formats[0].format,
            .imageColorSpace       = supported_formats[0].colorSpace,
            .imageExtent           = extent,
            .imageArrayLayers      = 1U,
            .imageUsage            = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0U,
            .pQueueFamilyIndices   = nullptr,
            .preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = supported_present_modes[0],
            .clipped               = VK_TRUE,
            .oldSwapchain          = nullptr,
    };

    VKCHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain));
}

void vkdevice::create_virtual_frames() {
    for (auto index{ 0U }; index < virtual_frames_count; index++) {
        {
            VkSemaphoreTypeCreateInfo semaphore_type_create_info{
                .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
                .pNext         = nullptr,
                .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
                .initialValue  = 0U,
            };

            VkSemaphoreCreateInfo create_info{
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                .pNext = &semaphore_type_create_info,
                .flags = 0U,
            };

            VKCHECK(vkCreateSemaphore(device, &create_info, nullptr, &semaphores[index]));
        }
    }
}

// ----------------- vkadapter -----------------

void vkdevice::create_instance() {
    VkApplicationInfo app_info{
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "path-tracer",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "gpu-path-tracer",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    constexpr const char* instance_layers[]{
        "VK_LAYER_KHRONOS_validation",
    };

    constexpr const char* instance_extensions[]{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0U,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = sizeof(instance_layers) / sizeof(instance_layers[0]),
        .ppEnabledLayerNames     = instance_layers,
        .enabledExtensionCount   = sizeof(instance_extensions) / sizeof(instance_extensions[0]),
        .ppEnabledExtensionNames = instance_extensions,
    };

    VKCHECK(vkCreateInstance(&create_info, nullptr, &instance));

    load_instance_functions(instance);
}

void vkdevice::create_device() {
    pick_physical_device();

    float                   queue_priority = 1.f;
    VkDeviceQueueCreateInfo queue_create_infos[static_cast<uint32_t>(QueueType::MAX)];

    for (auto& queue_create_info : queue_create_infos) {
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.pNext            = nullptr;
        queue_create_info.flags            = 0U;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
    }

    auto queue_properties_count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties{ queue_properties_count };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties.data());

    for (auto index{ 0U }; index < static_cast<uint32_t>(QueueType::MAX); index++) {
        auto& queue = queues[index];

        for (auto properties_index{ 0U }; properties_index < queue_properties_count; properties_index++) {
            auto& queue_property = queue_properties[properties_index];

            if ((queue_property.queueFlags & queue.usages) == queue.usages) {
                queue_create_infos[index].queueFamilyIndex = queue.index = properties_index;
                queue_property.queueFlags                                = 0U;
            }
        }
    }

    constexpr const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME
    };

    VkPhysicalDeviceVulkan12Features physical_device_12_features{
        .sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext                                     = nullptr,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
        .descriptorBindingPartiallyBound           = VK_TRUE,
        .runtimeDescriptorArray                    = VK_TRUE,
        .timelineSemaphore                         = VK_TRUE,
        .bufferDeviceAddress                       = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features physical_device_13_features{
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &physical_device_12_features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &physical_device_13_features,
        .flags                   = 0U,
        .queueCreateInfoCount    = static_cast<uint32_t>(QueueType::MAX),
        .pQueueCreateInfos       = queue_create_infos,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = sizeof(device_extensions) / sizeof(device_extensions[0]),
        .ppEnabledExtensionNames = device_extensions,
        .pEnabledFeatures        = nullptr,
    };

    VKCHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));

    load_device_functions(device);

    for (auto& queue : queues) {
        vkGetDeviceQueue(device, queue.index, 0, &queue.vk_queue);
    }

    create_command_pools();
}

void vkdevice::create_memory_allocator() {
    VmaAllocatorCreateInfo create_info{
        .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice   = physical_device,
        .device           = device,
        .instance         = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };

    VKCHECK(vmaCreateAllocator(&create_info, &gpu_allocator));
}

void vkdevice::create_command_pools() {
    VkCommandPoolCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    for (auto& queue : queues) {
        create_info.queueFamilyIndex = queue.index;

        VKCHECK(vkCreateCommandPool(device, &create_info, nullptr, &queue.command_pool));
    }
}

void vkdevice::pick_physical_device() {
    auto physical_devices_count = 0U;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);

    assert(physical_devices_count > 0);

    std::vector<VkPhysicalDevice> physical_devices{ physical_devices_count };
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data());

    for (auto& physical_dev : physical_devices) {
        if (device_support_features(physical_dev)) {
            physical_device = physical_dev;
            return;
        }
    }

    assert(true);
}

bool vkdevice::device_support_features(VkPhysicalDevice physical_dev) {
    // Vulkan 1.2 physical device features support
    {
        VkPhysicalDeviceVulkan12Features vulkan_12_features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };

        VkPhysicalDeviceFeatures2 physical_device_features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &vulkan_12_features,
        };

        vkGetPhysicalDeviceFeatures2(physical_dev, &physical_device_features);

        // clang-format off
        if (vulkan_12_features.bufferDeviceAddress                          == VK_FALSE ||
            vulkan_12_features.runtimeDescriptorArray                       == VK_FALSE ||
            vulkan_12_features.shaderStorageImageArrayNonUniformIndexing    == VK_FALSE ||
            vulkan_12_features.shaderSampledImageArrayNonUniformIndexing    == VK_FALSE ||
            vulkan_12_features.descriptorBindingPartiallyBound              == VK_FALSE ||
            vulkan_12_features.descriptorBindingUpdateUnusedWhilePending    == VK_FALSE ||
            vulkan_12_features.imagelessFramebuffer                         == VK_FALSE) {
            return false;
        }
        // clang-format on
    }

    // Vulkan 1.3 physical device features support
    {
        VkPhysicalDeviceVulkan13Features vulkan_13_features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        };

        VkPhysicalDeviceFeatures2 physical_device_features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &vulkan_13_features,
        };

        vkGetPhysicalDeviceFeatures2(physical_dev, &physical_device_features);

        if (vulkan_13_features.synchronization2 == VK_FALSE ||
            vulkan_13_features.dynamicRendering == VK_FALSE) {
            return false;
        }
    }

    return true;
}

void vkdevice::create_debug_layer_callback() {
    VkDebugUtilsMessengerCreateInfoEXT create_info{
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0U,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData       = nullptr,
    };

    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger));
}
