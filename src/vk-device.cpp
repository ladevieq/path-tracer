#include "vk-device.hpp"
#include "vk-device-types.hpp"
#include "vulkan-loader.hpp"

#include <vk_mem_alloc.h>

device_semaphore create_semaphore(VkDevice device, const semaphore_desc& desc, const char* name) {
    device_semaphore semaphore {
        .value = desc.initial_value,
    };
    VkSemaphoreTypeCreateInfo semaphore_type_create_info{
        .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
        .pNext         = nullptr,
        .semaphoreType = desc.type,
        .initialValue  = desc.initial_value,
    };

    VkSemaphoreCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &semaphore_type_create_info,
        .flags = 0U,
    };

    VKCHECK(vkCreateSemaphore(device, &create_info, nullptr, &semaphore.vk_semaphore));

    if (name) {
        VkDebugUtilsObjectNameInfoEXT object_name{
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext = nullptr,
            .objectType = VK_OBJECT_TYPE_SEMAPHORE,
            .objectHandle = reinterpret_cast<uint64_t>(semaphore.vk_semaphore),
            .pObjectName = name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &object_name);
    }
    return semaphore;
}


static VkShaderModule create_shader_module(VkDevice device, uint8_t* code, size_t code_size) {
    VkShaderModule shader_module;
    VkShaderModuleCreateInfo create_info{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0U,
        .codeSize = code_size,
        .pCode    = reinterpret_cast<uint32_t*>(code),
    };

    VKCHECK(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
    return shader_module;
}


vkdevice vkdevice::render_device = vkdevice();

void vkdevice::init() {
    queues[static_cast<uint32_t>(QueueType::GRAPHICS)] = {
        .usages = VK_QUEUE_GRAPHICS_BIT |
                  VK_QUEUE_COMPUTE_BIT |
                  VK_QUEUE_TRANSFER_BIT,
    };
    queues[static_cast<uint32_t>(QueueType::ASYNC_COMPUTE)] = {
        .usages = VK_QUEUE_COMPUTE_BIT,
    };
    queues[static_cast<uint32_t>(QueueType::TRANSFER)] = {
        .usages = VK_QUEUE_TRANSFER_BIT,
    };

    load_vulkan();

    create_instance();

    // create debug layer callback
#ifdef _DEBUG
    {
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
#endif // _DEBUG

    create_device();

    fence = create_semaphore(
        device,
        {
            .type = VK_SEMAPHORE_TYPE_TIMELINE,
        }, "timeline_semaphore"
    );
}

void vkdevice::deinit() {
    vkDeviceWaitIdle(device);

    vmaDestroyAllocator(gpu_allocator);

    for (auto& queue : queues) {
        vkDestroyCommandPool(device, queue.command_pool, nullptr);
    }

    vkDestroySemaphore(device, fence.vk_semaphore, nullptr);

    vkDestroyDevice(device, nullptr);

#ifdef _DEBUG
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
#endif // _DEBUG

    vkDestroyInstance(instance, nullptr);
}


void vkdevice::wait() {
    vkDeviceWaitIdle(device);
};

void vkdevice::wait_for_fence(u64 value) {
    VkSemaphoreWaitInfo semaphore_wait_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .semaphoreCount = 1U,
        .pSemaphores    = &fence.vk_semaphore,
        .pValues        = &value,
    };

    VKCHECK(vkWaitSemaphores(device, &semaphore_wait_info, UINT64_MAX));
}

handle<device_texture> vkdevice::create_texture(const texture_desc& desc) {
    device_texture device_texture;
    device_texture.init(desc);

    return textures.add(device_texture);
}

handle<device_sampler>   vkdevice::create_sampler(const sampler_desc& desc) {
    device_sampler device_sampler;
    device_sampler.init(desc);

    return samplers.add(device_sampler);
}

handle<device_buffer> vkdevice::create_buffer(const buffer_desc& desc) {
    device_buffer device_buffer{
        .size   = desc.size,
        .usages = desc.usages,
        .name   = desc.name,
    };

    VkBufferUsageFlags2CreateInfo usage_flags2 {
        .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR,
        .usage = desc.usages,
    };
    VkBufferCreateInfo create_info{
        .sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext                 = &usage_flags2,
        .flags                 = 0U,
        .size                  = desc.size,
        .usage                 = 0U,
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

    VmaAllocationInfo alloc_info{
        .pName = desc.name,
    };

    VKCHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.vk_buffer, &device_buffer.alloc, &alloc_info));
    vmaSetAllocationName(render_device.get_allocator(), device_buffer.alloc, desc.name);

    if ((desc.usages & VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT) != 0U) {
        VkBufferDeviceAddressInfo address_info {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .pNext = nullptr,
            .buffer = device_buffer.vk_buffer,
        };
        device_buffer.device_address = vkGetBufferDeviceAddress(device, &address_info);
    }

    device_buffer.mapped_ptr = alloc_info.pMappedData;

    return buffers.add(device_buffer);
}

handle<device_pipeline> vkdevice::create_graphics_pipeline(const graphics_pipeline_desc& desc) {
    device_pipeline device_pipeline{
    };

    VkPipelineShaderStageCreateInfo shader_stages_create_info[2U]{ {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0U,
            .stage               = VK_SHADER_STAGE_VERTEX_BIT,
            .module              = create_shader_module(device, desc.vs_code, desc.vs_code_size),
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0U,
            .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module              = create_shader_module(device, desc.fs_code, desc.fs_code_size),
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        },
    };

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
        .colorAttachmentCount    = static_cast<uint32_t>(desc.color_attachments_format_count),
        .pColorAttachmentFormats = desc.color_attachments_format,
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
        // .layout              = bindless.global_layout,
        .renderPass          = nullptr,
        .subpass             = 0U,
        .basePipelineHandle  = nullptr,
        .basePipelineIndex   = 0,
    };

    device_pipeline.bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
    VKCHECK(vkCreateGraphicsPipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));

    for(auto& stage_create_info : shader_stages_create_info) {
        vkDestroyShaderModule(device, stage_create_info.module, nullptr);
    }

    if (desc.name) {
        VkDebugUtilsObjectNameInfoEXT object_name{
            .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext          = nullptr,
            .objectType     = VK_OBJECT_TYPE_PIPELINE,
            .objectHandle   = reinterpret_cast<uint64_t>(device_pipeline.vk_pipeline),
            .pObjectName    = desc.name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &object_name);
    }

    return pipelines.add(device_pipeline);
}

handle<device_pipeline> vkdevice::create_compute_pipeline(const compute_pipeline_desc& desc) {
    device_pipeline device_pipeline{
    };

    VkPipelineShaderStageCreateInfo stage_create_info{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0U,
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = create_shader_module(device, desc.cs_code, desc.cs_code_size),
        .pName               = "main",
        .pSpecializationInfo = nullptr,
    };

    VkComputePipelineCreateInfo create_info{
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0U,
        .stage              = stage_create_info,
        // .layout             = bindless.global_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex  = 0,
    };

    device_pipeline.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    VKCHECK(vkCreateComputePipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));

    vkDestroyShaderModule(device, stage_create_info.module, nullptr);

    if (desc.name) {
        VkDebugUtilsObjectNameInfoEXT object_name{
            .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext          = nullptr,
            .objectType     = VK_OBJECT_TYPE_PIPELINE,
            .objectHandle   = reinterpret_cast<uint64_t>(device_pipeline.vk_pipeline),
            .pObjectName    = desc.name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &object_name);
    }

    return pipelines.add(device_pipeline);
}

handle<device_surface> vkdevice::create_surface(const surface_desc& desc) {
    device_surface device_surface {
    };

    {
        VkWin32SurfaceCreateInfoKHR create_info{
            .sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext     = nullptr,
            .flags     = 0U,
            .hinstance = GetModuleHandle(nullptr),
            .hwnd      = desc.window_handle,
        };

        VKCHECK(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &device_surface.vk_surface));
    }

    create_swapchain(desc, device_surface);

    assert(surface_desc::max_image_count >= device_surface.image_count);

    for (uint32_t index = 0u; index < device_surface.image_count; index++) {
        device_surface.acquire_semaphores[index] = create_semaphore(
            device,
            {
                .type = VK_SEMAPHORE_TYPE_BINARY,
            }, "acquire_semaphore"
        );
        device_surface.submit_semaphores[index]  = create_semaphore(
            device,
            {
                .type = VK_SEMAPHORE_TYPE_BINARY,
            }, "submit_semaphore"
        );
    }

    return surfaces.add(device_surface);
}

void vkdevice::destroy_texture(handle<device_texture> handle) {
    device_texture* texture = get_texture(handle);

    texture->release();
    textures.remove(handle);
}

void vkdevice::destroy_buffer(handle<device_buffer> handle) {
    device_buffer* buffer = get_buffer(handle);

    vmaDestroyBuffer(gpu_allocator, buffer->vk_buffer, buffer->alloc);
    buffers.remove(handle);
}

void vkdevice::destroy_pipeline(handle<device_pipeline> handle) {
    const device_pipeline* pipeline = get_pipeline(handle);

    vkDestroyPipeline(device, pipeline->vk_pipeline, nullptr);
    pipelines.remove(handle);
}

void vkdevice::destroy_surface(handle<device_surface> handle) {
    device_surface* surface = get_surface(handle);

    for (u32 index = 0U; index < surface->image_count; index++) {
        destroy_texture(surface->swapchain_images[index]);

        vkDestroySemaphore(device, surface->acquire_semaphores[index].vk_semaphore, nullptr);
        vkDestroySemaphore(device, surface->submit_semaphores[index].vk_semaphore, nullptr);
    }

    vkDestroySwapchainKHR(device, surface->vk_swapchain, nullptr);

    vkDestroySurfaceKHR(instance, surface->vk_surface, nullptr);
}

uint64_t vkdevice::submit(command_buffer* cmd_buffers, uint32_t count) {
    assert((max_submitable_command_buffers - count) >= 0);
    const auto queue_type = cmd_buffers[0].queue_type;

    // VkCommandBufferSubmitInfo command_buffers_info[count];
    VkCommandBufferSubmitInfo command_buffers_info[max_submitable_command_buffers];

    for (auto index{ 0U }; index < count; index++) {
        assert(queue_type == cmd_buffers[index].queue_type);

        auto& command_buffer_info         = command_buffers_info[index];
        command_buffer_info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.pNext         = nullptr;
        command_buffer_info.commandBuffer = cmd_buffers[index].vk_command_buffer;
        command_buffer_info.deviceMask    = 0U;
    }

    VkSubmitInfo2 submit_info{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0U,
        .waitSemaphoreInfoCount   = 0U,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = count,
        .pCommandBufferInfos      = command_buffers_info,
    };

    VkSemaphoreSubmitInfo signal_semaphore_infos[] = {
        {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext       = nullptr,
            .semaphore   = fence.vk_semaphore,
            .value       = ++fence.value,
            .stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0U,
        },
    };
    submit_info.signalSemaphoreInfoCount = sizeof(signal_semaphore_infos) / sizeof(signal_semaphore_infos[0U]);
    submit_info.pSignalSemaphoreInfos = signal_semaphore_infos;

    VKCHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));

    return fence.value;
}

uint64_t vkdevice::submit_before_present(handle<device_surface> surface_handle, command_buffer* cmd_buffers, uint32_t count) {
    assert((max_submitable_command_buffers - count) >= 0);
    const QueueType queue_type = cmd_buffers[0].queue_type;

    // VkCommandBufferSubmitInfo command_buffers_info[count];
    VkCommandBufferSubmitInfo command_buffers_info[max_submitable_command_buffers];

    for (uint32_t index{ 0U }; index < count; index++) {
        assert(queue_type == cmd_buffers[index].queue_type);

        auto& command_buffer_info         = command_buffers_info[index];
        command_buffer_info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        command_buffer_info.pNext         = nullptr;
        command_buffer_info.commandBuffer = cmd_buffers[index].vk_command_buffer;
        command_buffer_info.deviceMask    = 0U;
    }

    const device_surface* surface = get_surface(surface_handle);
    const device_semaphore& wait = surface->acquire_semaphores[surface->frame_index];
    VkSemaphoreSubmitInfo wait_semaphore_info = {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext       = nullptr,
            .semaphore   = wait.vk_semaphore,
            .value       = wait.value,
            .stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0U,
    };

    VkSubmitInfo2 submit_info{
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0U,
        .waitSemaphoreInfoCount   = 1U,
        .pWaitSemaphoreInfos      = &wait_semaphore_info,
        .commandBufferInfoCount   = count,
        .pCommandBufferInfos      = command_buffers_info,
    };

    const device_semaphore& signal = surface->submit_semaphores[surface->frame_index];
    VkSemaphoreSubmitInfo signal_semaphore_infos[] = {
        {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext       = nullptr,
            .semaphore   = fence.vk_semaphore,
            .value       = ++fence.value,
            .stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0U,
        },
        {
            .sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .pNext       = nullptr,
            .semaphore   = signal.vk_semaphore,
            .value       = signal.value,
            .stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .deviceIndex = 0U,
        },
    };
    submit_info.signalSemaphoreInfoCount = sizeof(signal_semaphore_infos) / sizeof(signal_semaphore_infos[0U]);
    submit_info.pSignalSemaphoreInfos = signal_semaphore_infos;

    VKCHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));

    return fence.value;
}

void vkdevice::present(handle<device_surface> surface_handle) {
    device_surface* surface = render_device.get_surface(surface_handle);
    VkResult result;
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .swapchainCount = 1U,
        .pSwapchains = &surface->vk_swapchain,
        .pImageIndices = &surface->image_index,
        .pResults = &result,
    };

    const device_semaphore& semaphore = surface->submit_semaphores[surface->frame_index];
    present_info.waitSemaphoreCount = 1U;
    present_info.pWaitSemaphores = &semaphore.vk_semaphore;

    VKCHECK(vkQueuePresentKHR(queues[static_cast<uint32_t>(QueueType::GRAPHICS)].vk_queue, &present_info));

    VKCHECK(result);
    surface->frame_index = (surface->frame_index + 1) % surface->image_count;
}

// private
void vkdevice::allocate_command_buffers(command_buffer* cmd_buffers, size_t count, QueueType type, const char* name) {
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

    for (uint32_t index{ 0U }; index < count; index++) {
        cmd_buffers[index].vk_command_buffer = command_buffers[index];
        cmd_buffers[index].queue_type        = type;

        if (name) {
            VkDebugUtilsObjectNameInfoEXT object_name{
                .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
                .pNext          = nullptr,
                .objectType     = VK_OBJECT_TYPE_COMMAND_BUFFER,
                .objectHandle   = reinterpret_cast<uint64_t>(command_buffers[index]),
                .pObjectName    = name,
            };
            vkSetDebugUtilsObjectNameEXT(device, &object_name);
        }
    }
}

void vkdevice::create_swapchain(const surface_desc& desc, device_surface& surface) {
    VkSurfaceKHR vk_surface = surface.vk_surface;

    auto&    queue     = queues[static_cast<uint32_t>(QueueType::GRAPHICS)];

    {
        VkBool32 supported = VK_FALSE;
        VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue.index, vk_surface, &supported));

        assert(supported);
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vk_surface, &surface_capabilities));

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

    {
        bool supported = false;
        u32 supported_formats_count = 0U;
        VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, nullptr));

        VkSurfaceFormatKHR* supported_formats = static_cast<VkSurfaceFormatKHR*>(_malloca(supported_formats_count * sizeof(VkSurfaceFormatKHR)));
        VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, supported_formats));

        for (u32 format_index = 0U; format_index < supported_formats_count; format_index++) {
            VkSurfaceFormatKHR surface_format = supported_formats[format_index];
            if (surface_format.format == desc.surface_format.format &&
                surface_format.colorSpace == desc.surface_format.colorSpace) {
                supported = true;
                break;
            }
        }

        assert(supported);
    }

    {
        bool supported = false;
        u32 supported_present_modes_count = 0U;
        VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, nullptr));

        VkPresentModeKHR* supported_present_modes = static_cast<VkPresentModeKHR*>(_malloca(supported_present_modes_count * sizeof(VkPresentModeKHR)));
        VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, supported_present_modes));

        for (u32 present_mode_index = 0U; present_mode_index < supported_present_modes_count; present_mode_index++) {
            VkPresentModeKHR present_mode = supported_present_modes[present_mode_index];
            if (present_mode == desc.present_mode) {
                supported = true;
                break;
            }
        }

        assert(supported);
    }

    assert(desc.image_count >= surface_capabilities.minImageCount);
    assert(desc.image_count <= surface_capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR create_info     = {
            .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext                 = nullptr,
            .flags                 = 0U,
            .surface               = vk_surface,
            .minImageCount         = desc.image_count,
            .imageFormat           = desc.surface_format.format,
            .imageColorSpace       = desc.surface_format.colorSpace,
            .imageExtent           = extent,
            .imageArrayLayers      = 1U,
            .imageUsage            = desc.usages,
            .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0U,
            .pQueueFamilyIndices   = nullptr,
            .preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode           = desc.present_mode,
            .clipped               = VK_TRUE,
            .oldSwapchain          = nullptr,
    };

    VKCHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &surface.vk_swapchain));

    surface.surface_format = desc.surface_format;

    vkGetSwapchainImagesKHR(device, surface.vk_swapchain, &surface.image_count, nullptr);
    assert(surface.image_count <= surface_desc::max_image_count);

    VkImage* images = static_cast<VkImage*>(_malloca(surface.image_count * sizeof(VkImage)));
    vkGetSwapchainImagesKHR(device, surface.vk_swapchain, &surface.image_count, images);

    for (u32 index = 0U; index < surface.image_count; index++) {
        surface.swapchain_images[index] = create_texture({
            .width = extent.width,
            .height = extent.height,
            .depth = 1U,
            .mips = 1U,
            .usages = desc.usages,
            .format = desc.surface_format.format,
            .type = VK_IMAGE_TYPE_2D,
            .vk_image = images[index],
            .name = "swapchain image"
        });
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

    const char* instance_layers[]{
        "VK_LAYER_KHRONOS_validation",
    };

    const char* instance_extensions[]{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
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
    // pick physical device
    // TODO: change put this in another scope
    VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptor_heap_props {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT,
    };
    {
        u32 physical_devices_count = 0U;
        vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);
        assert(physical_devices_count > 0);

        VkPhysicalDevice* physical_devices = static_cast<VkPhysicalDevice*>(_malloca(physical_devices_count * sizeof(VkPhysicalDevice)));
        vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices);

        physical_device_props = VkPhysicalDeviceProperties2 {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &descriptor_heap_props,
        };

        for (u32 physical_device_index = 0U; physical_device_index < physical_devices_count; physical_device_index++) {
            VkPhysicalDevice physical_dev = physical_devices[physical_device_index];
            if (device_support_features(physical_dev)) {
                physical_device = physical_dev;
                break;
            }
        }

        assert(true);
    }

    float                   queue_priority = 1.f;
    VkDeviceQueueCreateInfo queue_create_infos[static_cast<uint32_t>(QueueType::MAX)];

    for (auto& queue_create_info : queue_create_infos) {
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.pNext            = nullptr;
        queue_create_info.flags            = 0U;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
    }

    u32 queue_properties_count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    VkQueueFamilyProperties* queue_properties = static_cast<VkQueueFamilyProperties*>(_malloca(queue_properties_count * sizeof(VkQueueFamilyProperties)));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties);

    for (u32 index = 0U; index < static_cast<uint32_t>(QueueType::MAX); index++) {
        queue& queue = queues[index];

        for (u32 properties_index = 0U; properties_index < queue_properties_count; properties_index++) {
            VkQueueFamilyProperties queue_property = queue_properties[properties_index];

            if ((queue_property.queueFlags & queue.usages) == queue.usages) {
                queue_create_infos[index].queueFamilyIndex = queue.index = properties_index;
                queue_property.queueFlags                                = 0U;
            }
        }
    }

    const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
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

    // create command pool
    {
        VkCommandPoolCreateInfo command_pool_create_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };

        for (queue& queue : queues) {
            command_pool_create_info.queueFamilyIndex = queue.index;

            VKCHECK(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &queue.command_pool));
        }
    }

    // create memory allocator
    {
        VmaVulkanFunctions functions = {
            .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
            .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        };
        VmaAllocatorCreateInfo allocator_create_info{
            .flags            = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
            .physicalDevice   = physical_device,
            .device           = device,
            .pVulkanFunctions = &functions,
            .instance         = instance,
            .vulkanApiVersion = VK_API_VERSION_1_2,
        };

        VKCHECK(vmaCreateAllocator(&allocator_create_info, &gpu_allocator));
    }

    create_heaps(descriptor_heap_props);
}

void vkdevice::create_heaps(VkPhysicalDeviceDescriptorHeapPropertiesEXT& descriptor_heap_props) {
    // create image heap
    {
        sampler_heap = create_buffer({
            .size = descriptor_heap_props.minResourceHeapReservedRange + max_images_count * descriptor_heap_props.imageDescriptorSize,
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                      VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT |
                      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
            .memory_usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .name = "image descriptor_heap",
        });
    }

    // create sampler heap
    {
        sampler_heap = create_buffer({
            .size = descriptor_heap_props.minSamplerHeapReservedRange + max_samplers_count * descriptor_heap_props.samplerDescriptorSize,
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                      VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT |
                      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
            .memory_usage = VMA_MEMORY_USAGE_GPU_ONLY,
            .name = "sampler descriptor_heap",
        });
    }
}


bool vkdevice::device_support_features(VkPhysicalDevice physical_dev) {
    // limits
    vkGetPhysicalDeviceProperties2(physical_dev, &physical_device_props);

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

        if (vulkan_12_features.bufferDeviceAddress                          == VK_FALSE ||
            vulkan_12_features.runtimeDescriptorArray                       == VK_FALSE ||
            vulkan_12_features.shaderStorageImageArrayNonUniformIndexing    == VK_FALSE ||
            vulkan_12_features.shaderSampledImageArrayNonUniformIndexing    == VK_FALSE ||
            vulkan_12_features.descriptorBindingPartiallyBound              == VK_FALSE ||
            vulkan_12_features.descriptorBindingUpdateUnusedWhilePending    == VK_FALSE ||
            vulkan_12_features.imagelessFramebuffer                         == VK_FALSE) {
            return false;
        }
    }

    // Vulkan 1.3 physical device features support
    {
        VkPhysicalDeviceVulkan13Features vulkan_13_features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = nullptr,
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


idlist<> device_texture::sampled_indices = {};
idlist<> device_texture::storage_indices = {};
idlist<> device_sampler::sampler_indices = {};

void device_texture::init(const texture_desc& desc) {
    auto& render_device = vkdevice::get();
    auto* device = render_device.get_device();

    vk_image = desc.vk_image;
    format = desc.format;
    width = desc.width;
    height = desc.height;
    mips = desc.mips;
    name = desc.name;
    aspects = ((desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
        ? VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;

    assert(mips <= texture_desc::max_mips);

    if (vk_image == nullptr) {
        VkImageCreateInfo create_info{
            .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext     = nullptr,
            .flags     = 0U,
            .imageType = desc.type,
            .format    = desc.format,
            .extent    = {
                   .width  = desc.width,
                   .height = desc.height,
                   .depth  = desc.depth
            },
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

        VKCHECK(vmaCreateImage(render_device.get_allocator(), &create_info, &alloc_create_info, &vk_image, &alloc, nullptr));
        vmaSetAllocationName(render_device.get_allocator(), alloc, desc.name);
    }

    if (desc.name) {
        VkDebugUtilsObjectNameInfoEXT object_name{
            .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext          = nullptr,
            .objectType     = VK_OBJECT_TYPE_IMAGE,
            .objectHandle   = reinterpret_cast<uint64_t>(vk_image),
            .pObjectName    = desc.name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &object_name);
    }

    VkImageViewCreateInfo image_view_create_info{
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext      = nullptr,
        .flags      = 0U,
        .image      = vk_image,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask     = aspects,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };
    // create views
    {
        for (u32 mip_index = 0U; mip_index < mips; mip_index++) {
            image_view_create_info.subresourceRange.baseMipLevel = mip_index;
            VKCHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &mips_views[mip_index]));
        }

        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount   = mips;
        VKCHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &whole_view));
    }

    // TODO: Handle mips views
    VkImageDescriptorInfoEXT image_info = {
        .sType  = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .pNext  = nullptr,
        .pView  = &image_view_create_info,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    u32 descriptors_count = 0U;
    VkResourceDescriptorInfoEXT resource_descriptor_infos[2U];
    VkHostAddressRangeEXT resources_address[2U];
    if ((desc.usages & VK_IMAGE_USAGE_STORAGE_BIT) != 0) {
        resource_descriptor_infos[descriptors_count] = VkResourceDescriptorInfoEXT{
            .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .data  = {
                .pImage = &image_info,
            },
        };

        // resource_address[descriptors_count] = {
        //     .address = storage_indices.add(),
        //     .size    = ,
        // }

        descriptors_count++;
    }
    if ((desc.usages & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) {
        resource_descriptor_infos[descriptors_count] = VkResourceDescriptorInfoEXT{
            .sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .data  = {
                .pImage = &image_info,
            },
        };

        // resource_address[descriptors_count] = {
        //     .address = sampled_indices.add(),
        //     .size    = ,
        // }

        descriptors_count++;
    }

    VKCHECK(vkWriteResourceDescriptorsEXT(device, static_cast<uint32_t>(descriptors_count), resource_descriptor_infos, resources_address));
}

void device_texture::release() {
    vkdevice& render_device = vkdevice::get();
    VkDevice device = render_device.get_device();
    auto* gpu_allocator = render_device.get_allocator();

    if (alloc != nullptr) {
        vmaDestroyImage(gpu_allocator, vk_image, alloc);
    }

    vkDestroyImageView(device, whole_view, nullptr);

    for (auto mip_index{ 0U }; mip_index < mips; mip_index++) {
        const auto& mip_view = mips_views[mip_index];
        vkDestroyImageView(device, mip_view, nullptr);
    }

    if (storage_index.is_valid()) {
        storage_indices.remove(storage_index);
    }

    if (sampled_index.is_valid()) {
        sampled_indices.remove(sampled_index);
    }
}

void device_texture::create_views() {
}


void device_sampler::init(const sampler_desc& desc) {
    auto& render_device = vkdevice::get();
    auto* device = render_device.get_device();

    VkSamplerCreateInfo sampler_create_info{
        .sType                      = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext                      = nullptr,
        .flags                      = 0U,
        .magFilter                  = desc.mag_filter,
        .minFilter                  = desc.min_filter,
        .mipmapMode                 = desc.mipmap_mode,
        .addressModeU               = desc.address_mode_u,
        .addressModeV               = desc.address_mode_v,
        .addressModeW               = desc.address_mode_w,
        .mipLodBias                 = desc.mip_lod_bias,
        .anisotropyEnable           = desc.anisotropy,
        .maxAnisotropy              = desc.max_anisotropy,
        .compareEnable              = desc.compare_enabled,
        .compareOp                  = desc.compare_op,
        .minLod                     = desc.min_lod,
        .maxLod                     = desc.max_lod,
        .borderColor                = desc.border_color,
        .unnormalizedCoordinates    = desc.unnomalized_coords,
    };

    VKCHECK(vkCreateSampler(device, &sampler_create_info, nullptr, &vk_sampler));

    sampler_index = sampler_indices.add();

    // VkWriteDescriptorSet write_descriptor_set{
    //     .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //     .pNext            = nullptr,
    //     .dstSet           = bindless.sets[BindlessSetType::GLOBAL],
    //     .dstBinding       = 1U,
    //     .dstArrayElement  = sampler_index.id,
    //     .descriptorCount  = 1U,
    //     .descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    //     .pImageInfo       = images_info,
    //     .pBufferInfo      = nullptr,
    //     .pTexelBufferView = nullptr,
    // });

    // TODO: Write samplers to heap
    // TODO: Create previously immutable samplers
    VKCHECK(vkWriteSamplerDescriptorsEXT());
}


uint32_t vkdevice::acquire_image_index(handle<device_surface> surface_handle) {
    device_surface* surface = get_surface(surface_handle);
    const device_semaphore& signal = surface->acquire_semaphores[surface->frame_index];

    VkSemaphoreWaitInfo wait_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .semaphoreCount = 1U,
        .pSemaphores = &fence.vk_semaphore,
        .pValues = &fence.value,
    };
    vkWaitSemaphores(device, &wait_info, UINT64_MAX);
    VKCHECK(vkAcquireNextImageKHR(device, surface->vk_swapchain, UINT64_MAX, signal.vk_semaphore, nullptr, &surface->image_index));

    return surface->image_index;
}
