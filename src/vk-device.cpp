#include "vk-device.hpp"
#include "vk-device-types.hpp"
#include "vulkan-loader.hpp"

#include <vk_mem_alloc.h>


static VkInstance                  instance;
static VkPhysicalDevice            physical_device;
static VkPhysicalDeviceProperties2 physical_device_props;
static VkDevice                    device;

static constexpr size_t     max_images_count = 10000U;
static constexpr size_t     max_samplers_count = 16U;
static constexpr size_t     storage_images_offset = 0U;
static constexpr size_t     sampled_images_offset = max_images_count / 2U;

// TODO: Dont use handle for internally used resources
handle<device_buffer> image_heap;
handle<device_buffer> sampler_heap;

static VmaAllocator                gpu_allocator;

VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptor_heap_props {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT,
};

struct physical_device_features {
    VkPhysicalDeviceFeatures2                           physical_device_features2;
    VkPhysicalDeviceVulkan12Features                    physical_device_12_features;
    VkPhysicalDeviceVulkan13Features                    physical_device_13_features;
    VkPhysicalDeviceVulkan14Features                    physical_device_14_features;
    VkPhysicalDeviceDescriptorHeapFeaturesEXT           descriptor_heap_features;
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR    shader_untyped_pointers_features;
};

static void create_heaps(size_t max_images, size_t max_samplers) {
    // create image heap
    {
        u64 alignment = descriptor_heap_props.imageDescriptorAlignment < descriptor_heap_props.bufferDescriptorAlignment ?
            descriptor_heap_props.bufferDescriptorAlignment :
            descriptor_heap_props.imageDescriptorAlignment;

        image_heap = vkdevice::get().create_buffer({
            .size = descriptor_heap_props.minResourceHeapReservedRange + ALIGN(max_images * descriptor_heap_props.imageDescriptorSize, alignment),
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                      VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT |
                      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .name = "image descriptor_heap",
        });
    }

    // create sampler heap
    {
        sampler_heap = vkdevice::get().create_buffer({
            .size = descriptor_heap_props.minSamplerHeapReservedRange + ALIGN(max_samplers * descriptor_heap_props.samplerDescriptorSize, descriptor_heap_props.samplerDescriptorAlignment),
            .usages = VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                      VK_BUFFER_USAGE_2_DESCRIPTOR_HEAP_BIT_EXT |
                      VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
            .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .memory_usage = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            .name = "sampler descriptor_heap",
        });
    }
}

static device_semaphore create_semaphore(const semaphore_desc& desc, const char* name) {
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

    VK_CHECK(vkCreateSemaphore(device, &create_info, nullptr, &semaphore.vk_semaphore));

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


static VkShaderModule create_shader_module(uint8_t* code, size_t code_size) {
    VkShaderModule shader_module;
    VkShaderModuleCreateInfo create_info{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0U,
        .codeSize = code_size,
        .pCode    = reinterpret_cast<uint32_t*>(code),
    };

    VK_CHECK(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
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
        {
            .type = VK_SEMAPHORE_TYPE_TIMELINE,
        }, "timeline_semaphore"
    );
}

void vkdevice::deinit() {
    vkDeviceWaitIdle(device);

    destroy_buffer(image_heap);
    destroy_buffer(sampler_heap);

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

    VK_CHECK(vkWaitSemaphores(device, &semaphore_wait_info, UINT64_MAX));
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

    VK_CHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.vk_buffer, &device_buffer.alloc, &alloc_info));

    if (desc.name) {
        VkDebugUtilsObjectNameInfoEXT object_name{
            .sType          = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .pNext          = nullptr,
            .objectType     = VK_OBJECT_TYPE_BUFFER,
            .objectHandle   = reinterpret_cast<uint64_t>(device_buffer.vk_buffer),
            .pObjectName    = desc.name,
        };
        vkSetDebugUtilsObjectNameEXT(device, &object_name);
    }

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
            .module              = create_shader_module(desc.vs_code, desc.vs_code_size),
            .pName               = "main",
            .pSpecializationInfo = nullptr,
        },
        {
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0U,
            .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module              = create_shader_module(desc.fs_code, desc.fs_code_size),
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

    VkPipelineCreateFlags2CreateInfo create_flags2 {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .pNext = &pipeline_rendering_create_info,
        .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };

    VkGraphicsPipelineCreateInfo create_info{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &create_flags2,
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
    VK_CHECK(vkCreateGraphicsPipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));

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
        .module              = create_shader_module(desc.cs_code, desc.cs_code_size),
        .pName               = "main",
        .pSpecializationInfo = nullptr,
    };

    VkPipelineCreateFlags2CreateInfo create_flags2 {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };

    VkComputePipelineCreateInfo create_info{
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = &create_flags2,
        .flags              = 0U,
        .stage              = stage_create_info,
        // .layout             = bindless.global_layout,
        .basePipelineHandle = nullptr,
        .basePipelineIndex  = 0,
    };

    device_pipeline.bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
    VK_CHECK(vkCreateComputePipelines(device, nullptr, 1U, &create_info, nullptr, &device_pipeline.vk_pipeline));

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

        VK_CHECK(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &device_surface.vk_surface));
    }

    create_swapchain(desc, device_surface);

    assert(surface_desc::max_image_count >= device_surface.image_count);

    for (uint32_t index = 0u; index < device_surface.image_count; index++) {
        device_surface.acquire_semaphores[index] = create_semaphore(
            {
                .type = VK_SEMAPHORE_TYPE_BINARY,
            }, "acquire_semaphore"
        );
        device_surface.submit_semaphores[index]  = create_semaphore(
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

    VK_CHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));

    return fence.value;
}

uint64_t vkdevice::submit_before_present(handle<device_surface> surface_handle, command_buffer* cmd_buffers, uint32_t count) {
    assert((max_submitable_command_buffers - count) >= 0);
    const QueueType queue_type = cmd_buffers[0].queue_type;

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

    VK_CHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));

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

    VK_CHECK(vkQueuePresentKHR(queues[static_cast<uint32_t>(QueueType::GRAPHICS)].vk_queue, &present_info));

    VK_CHECK(result);
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

    VK_CHECK(vkAllocateCommandBuffers(device, &allocate_info, command_buffers));

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
        VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue.index, vk_surface, &supported));

        assert(supported);
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, vk_surface, &surface_capabilities));

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
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, nullptr));

        VkSurfaceFormatKHR* supported_formats = static_cast<VkSurfaceFormatKHR*>(_malloca(supported_formats_count * sizeof(VkSurfaceFormatKHR)));
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, supported_formats));

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
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, nullptr));

        VkPresentModeKHR* supported_present_modes = static_cast<VkPresentModeKHR*>(_malloca(supported_present_modes_count * sizeof(VkPresentModeKHR)));
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, supported_present_modes));

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

    VK_CHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &surface.vk_swapchain));

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

    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));

    load_instance_functions(instance);
}

void vkdevice::create_device() {
    // pick physical device
    struct physical_device_features features = {
        .physical_device_features2 = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &features.physical_device_12_features,
        },
        .physical_device_12_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
            .pNext = &features.physical_device_13_features,
        },
        .physical_device_13_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &features.physical_device_14_features,
        },
        .physical_device_14_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES,
            .pNext = &features.descriptor_heap_features,
        },
        .descriptor_heap_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
            .pNext = &features.shader_untyped_pointers_features,
        },
        .shader_untyped_pointers_features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
            .pNext = nullptr,
        },
    };

    // select physical device
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
            vkGetPhysicalDeviceProperties2(physical_dev, &physical_device_props);
            vkGetPhysicalDeviceFeatures2(physical_dev, &features.physical_device_features2);

            if (features.physical_device_12_features.timelineSemaphore                            == VK_TRUE &&
                features.physical_device_12_features.bufferDeviceAddress                          == VK_TRUE &&
                features.physical_device_12_features.runtimeDescriptorArray                       == VK_TRUE &&
                features.physical_device_12_features.shaderStorageImageArrayNonUniformIndexing    == VK_TRUE &&
                features.physical_device_12_features.shaderSampledImageArrayNonUniformIndexing    == VK_TRUE &&
                features.physical_device_12_features.descriptorBindingPartiallyBound              == VK_TRUE &&
                features.physical_device_12_features.descriptorBindingUpdateUnusedWhilePending    == VK_TRUE &&
                features.physical_device_12_features.imagelessFramebuffer                         == VK_TRUE &&
                features.physical_device_13_features.synchronization2                             == VK_TRUE &&
                features.physical_device_13_features.dynamicRendering                             == VK_TRUE &&
                features.physical_device_14_features.maintenance5                                 == VK_TRUE &&
                features.descriptor_heap_features.descriptorHeap                                  == VK_TRUE &&
                features.descriptor_heap_features.descriptorHeapCaptureReplay                     == VK_TRUE &&
                features.shader_untyped_pointers_features.shaderUntypedPointers                   == VK_TRUE) {

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
        VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
    };

    VkDeviceCreateInfo create_info{
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &features.physical_device_features2,
        .flags                   = 0U,
        .queueCreateInfoCount    = static_cast<uint32_t>(QueueType::MAX),
        .pQueueCreateInfos       = queue_create_infos,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = nullptr,
        .enabledExtensionCount   = sizeof(device_extensions) / sizeof(device_extensions[0]),
        .ppEnabledExtensionNames = device_extensions,
        .pEnabledFeatures        = nullptr,
    };

    VK_CHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device));

    load_device_functions(device);

    for (struct queue& queue : queues) {
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

            VK_CHECK(vkCreateCommandPool(device, &command_pool_create_info, nullptr, &queue.command_pool));
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

        VK_CHECK(vmaCreateAllocator(&allocator_create_info, &gpu_allocator));
    }

    create_heaps(max_images_count, max_samplers_count);
}


idlist<> device_texture::sampled_indices = {};
idlist<> device_texture::storage_indices = {};
idlist<> device_sampler::sampler_indices = {};

void device_texture::init(const texture_desc& desc) {
    auto& render_device = vkdevice::get();

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

        VK_CHECK(vmaCreateImage(gpu_allocator, &create_info, &alloc_create_info, &vk_image, &alloc, nullptr));
        vmaSetAllocationName(gpu_allocator, alloc, desc.name);
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
            VK_CHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &mips_views[mip_index]));
        }

        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount   = mips;
        VK_CHECK(vkCreateImageView(device, &image_view_create_info, nullptr, &whole_view));
    }

    // TODO: Handle mips views
    VkImageDescriptorInfoEXT image_info = {
        .sType  = VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT,
        .pNext  = nullptr,
        .pView  = &image_view_create_info,
        .layout = VK_IMAGE_LAYOUT_GENERAL,
    };

    device_buffer* image_heap_buffer = render_device.get_buffer(image_heap);
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

        static u32 strg_index = 0U;
        strg_index++;
        storage_index = storage_indices.add();
        resources_address[descriptors_count] = {
            .address = ((u8*)image_heap_buffer->mapped_ptr) + descriptor_heap_props.imageDescriptorSize * (strg_index + storage_images_offset),
            .size    = descriptor_heap_props.imageDescriptorSize,
        };

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

        static u32 smpl_index = 0U;
        smpl_index++;
        sampled_index = sampled_indices.add();
        resources_address[descriptors_count] = {
            .address = ((u8*)image_heap_buffer->mapped_ptr) + descriptor_heap_props.imageDescriptorSize * (smpl_index + sampled_images_offset),
            .size    = descriptor_heap_props.imageDescriptorSize,
        };

        descriptors_count++;
    }

    VK_CHECK(vkWriteResourceDescriptorsEXT(device, static_cast<uint32_t>(descriptors_count), resource_descriptor_infos, resources_address));
}

void device_texture::release() {
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
    vkdevice& render_device = vkdevice::get();

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

    VK_CHECK(vkCreateSampler(device, &sampler_create_info, nullptr, &vk_sampler));

    sampler_index = sampler_indices.add();

    device_buffer* sampler_heap_buffer = render_device.get_buffer(sampler_heap);

    VkHostAddressRangeEXT destination_address {
        .address = ((u8*)sampler_heap_buffer->mapped_ptr) + descriptor_heap_props.samplerDescriptorSize * sampler_index.id,
        .size    = descriptor_heap_props.samplerDescriptorSize,
    };
    VK_CHECK(vkWriteSamplerDescriptorsEXT(device, 1U, &sampler_create_info, &destination_address));
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
    VK_CHECK(vkAcquireNextImageKHR(device, surface->vk_swapchain, UINT64_MAX, signal.vk_semaphore, nullptr, &surface->image_index));

    return surface->image_index;
}
