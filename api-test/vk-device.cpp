#include "vk-device.hpp"

#include <cassert>
#include <vector>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vk-utils.hpp"
#include "vulkan-loader.hpp"

vkdevice vkdevice::render_device = vkdevice();

void vkdevice::init() {
    queues[0U] = {
        .usages = VK_QUEUE_GRAPHICS_BIT |
                  VK_QUEUE_COMPUTE_BIT |
                  VK_QUEUE_TRANSFER_BIT,
    };
   queues[1U] = {
        .usages = VK_QUEUE_COMPUTE_BIT,
    };
    queues[2U] = {
        .usages = VK_QUEUE_TRANSFER_BIT,
    };

    load_vulkan();

    create_instance();

#ifdef _DEBUG
    create_debug_layer_callback();
#endif // _DEBUG

    create_device();

    create_memory_allocator();

    bindless = bindless_model::create_bindless_model();
}

vkdevice::~vkdevice() {
    vkDeviceWaitIdle(device);

    bindless_model::destroy_bindless_model(bindless);

    vmaDestroyAllocator(gpu_allocator);

    for (auto& queue : queues) {
        vkDestroyCommandPool(device, queue.command_pool, nullptr);
    }

    vkDestroyDevice(device, nullptr);

#ifdef _DEBUG
    vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
#endif // _DEBUG

    vkDestroyInstance(instance, nullptr);
}

handle<device_texture> vkdevice::create_texture(const texture_desc& desc) {
    device_texture device_texture;
    device_texture.init(desc);

    return textures.add(device_texture);
}

handle<device_buffer> vkdevice::create_buffer(const buffer_desc& desc) {
    device_buffer device_buffer{
        .size = desc.size,
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

    return buffers.add(device_buffer);
}

handle<device_pipeline> vkdevice::create_pipeline(const pipeline_desc& desc) {
    device_pipeline device_pipeline{
    };
    if (!desc.cs_code.empty()) {
        VkPipelineShaderStageCreateInfo stage_create_info{
            .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext               = nullptr,
            .flags               = 0U,
            .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
            .module              = create_shader_module(desc.cs_code),
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

        vkDestroyShaderModule(device, stage_create_info.module, nullptr);
    } else {
        VkPipelineShaderStageCreateInfo shader_stages_create_info[2U]{
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0U,
                .stage               = VK_SHADER_STAGE_VERTEX_BIT,
                .module              = create_shader_module(desc.vs_code),
                .pName               = "main",
                .pSpecializationInfo = nullptr,
            },
            {
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0U,
                .stage               = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module              = create_shader_module(desc.fs_code),
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

        for(auto& stage_create_info : shader_stages_create_info) {
            vkDestroyShaderModule(device, stage_create_info.module, nullptr);
        }
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

    return surfaces.add(device_surface);
}

handle<device_semaphore> vkdevice::create_semaphore(const semaphore_desc& desc) {
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

    return semaphores.add(semaphore);
}

void vkdevice::destroy_texture(handle<device_texture> handle) {
    auto& texture = get_texture(handle);

    texture.release();
    textures.remove(handle);
}

void vkdevice::destroy_buffer(handle<device_buffer> handle) {
    const auto& buffer = get_buffer(handle);

    vmaDestroyBuffer(gpu_allocator, buffer.vk_buffer, buffer.alloc);
    buffers.remove(handle);
}

void vkdevice::destroy_pipeline(handle<device_pipeline> handle) {
    const auto& pipeline = vkdevice::get_pipeline(handle);

    vkDestroyPipeline(device, pipeline.vk_pipeline, nullptr);
    pipelines.remove(handle);
}

void vkdevice::destroy_semaphore(handle<device_semaphore> handle) {
    auto& semaphore = vkdevice::get_semaphore(handle);

    vkDestroySemaphore(device, semaphore.vk_semaphore, nullptr);
    semaphores.remove(handle);
}

void vkdevice::destroy_surface(handle<device_surface> handle) {
    const auto& surface = vkdevice::get_surface(handle);

    for (auto texture_index{0U}; texture_index < surface.image_count; texture_index++) {
        auto texture_handle = surface.swapchain_images[texture_index];
        destroy_texture(texture_handle);
    }

    vkDestroySwapchainKHR(device, surface.vk_swapchain, nullptr);

    vkDestroySurfaceKHR(instance, surface.vk_surface, nullptr);
}

void vkdevice::wait(handle<device_semaphore> semaphore_handle) {
    const auto& semaphore = vkdevice::get_render_device().get_semaphore(semaphore_handle);
    VkSemaphoreWaitInfo wait_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
        .pNext = nullptr,
        .flags = 0U,
        .semaphoreCount = 1U,
        .pSemaphores = &semaphore.vk_semaphore,
        .pValues = &semaphore.value,
    };
    VKCHECK(vkWaitSemaphores(device, &wait_info, -1));
    vkDeviceWaitIdle(device);
}

void vkdevice::submit(std::span<command_buffer> buffers, handle<device_semaphore> wait_handle, handle<device_semaphore> signal_handle) {
    assert((max_submitable_command_buffers - buffers.size()) >= 0);
    const auto queue_type = buffers[0].queue_type;

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

    if (wait_handle.is_valid()) {
        const auto& wait = vkdevice::get_render_device().get_semaphore(wait_handle);
        wait_semaphore_info.semaphore      = wait.vk_semaphore;
        wait_semaphore_info.value          = wait.value;
        submit_info.waitSemaphoreInfoCount = 1U;
        submit_info.pWaitSemaphoreInfos    = &wait_semaphore_info;
    }

    if (signal_handle.is_valid()) {
        auto& signal = vkdevice::get_render_device().get_semaphore(signal_handle);
        signal_semaphore_info.semaphore      = signal.vk_semaphore;
        signal_semaphore_info.value          = ++signal.value;

        submit_info.signalSemaphoreInfoCount = 1U;
        submit_info.pSignalSemaphoreInfos    = &signal_semaphore_info;
    }

    VKCHECK(vkQueueSubmit2(queues[static_cast<uint32_t>(queue_type)].vk_queue, 1, &submit_info, nullptr));
}

void vkdevice::present(handle<device_surface> surface_handle, handle<device_semaphore> semaphore_handle) {
    const auto& surface = vkdevice::get_render_device().get_surface(surface_handle);
    VkResult result;
    VkPresentInfoKHR present_info{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .swapchainCount = 1U,
        .pSwapchains = &surface.vk_swapchain,
        .pImageIndices = &surface.image_index,
        .pResults = &result,
    };

    if (semaphore_handle.is_valid()) {
        const auto& semaphore = vkdevice::get_render_device().get_semaphore(semaphore_handle);
        present_info.waitSemaphoreCount = 1U;
        present_info.pWaitSemaphores = &semaphore.vk_semaphore;
    }

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

VkShaderModule vkdevice::create_shader_module(const std::span<uint8_t>& shader_code) {
    VkShaderModule shader_module;
    const auto&              code = shader_code;
    VkShaderModuleCreateInfo create_info{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = nullptr,
        .flags    = 0U,
        .codeSize = code.size(),
        .pCode    = reinterpret_cast<uint32_t*>(code.data()),
    };

    VKCHECK(vkCreateShaderModule(device, &create_info, nullptr, &shader_module));
    return shader_module;
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
        auto supported = false;
        auto supported_formats_count = 0U;
        VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, nullptr));

        std::vector<VkSurfaceFormatKHR> supported_formats{ supported_formats_count };
        VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, vk_surface, &supported_formats_count, supported_formats.data()));

        for (auto& surface_format : supported_formats) {
            if (surface_format.format == desc.surface_format.format &&
                surface_format.colorSpace == desc.surface_format.colorSpace) {
                supported = true;
                break;
            }
        }

        assert(supported);
    }

    {
        auto supported = false;
        auto supported_present_modes_count = 0U;
        VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, nullptr));

        std::vector<VkPresentModeKHR> supported_present_modes{ supported_present_modes_count };
        VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, vk_surface, &supported_present_modes_count, supported_present_modes.data()));

        for (auto& present_mode : supported_present_modes) {
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

    std::vector<VkImage> images {surface.image_count};
    vkGetSwapchainImagesKHR(device, surface.vk_swapchain, &surface.image_count, images.data());

    for (auto index {0U}; index < surface.image_count; index++) {
        surface.swapchain_images[index] = create_texture({
            .width = extent.width,
            .height = extent.height,
            .depth = 1U,
            .mips = 1U,
            .usages = desc.usages,
            .format = desc.surface_format.format,
            .type = VK_IMAGE_TYPE_2D,
            .vk_image = images[index],
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
#ifdef _DEBUG
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
#endif // _DEBUG
}


idlist<> device_texture::sampled_indices = {};
idlist<> device_texture::storage_indices = {};

void device_texture::init(const texture_desc& desc) {
    auto& render_device = vkdevice::get_render_device();
    const auto& bindless = render_device.get_bindingmodel();
    auto* device = render_device.get_device();

    vk_image = desc.vk_image;
    format = desc.format;
    width = desc.width;
    height = desc.height;
    mips = desc.mips;
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
    }

    create_views();

    // TODO: Handle mips views
    VkDescriptorImageInfo images_info[]{
        {
            nullptr,
            whole_view,
            VK_IMAGE_LAYOUT_GENERAL,
        },
    };

    std::vector<VkWriteDescriptorSet> write_descriptor_sets;
    if ((desc.usages & VK_IMAGE_USAGE_STORAGE_BIT) != 0) {
        storage_index = storage_indices.add();

        write_descriptor_sets.push_back({
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = bindless.sets[BindlessSetType::GLOBAL],
            .dstBinding       = 2U,
            .dstArrayElement  = storage_index.id,
            .descriptorCount  = sizeof(images_info) / sizeof(VkDescriptorImageInfo),
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo       = images_info,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr,
        });
    }
    if ((desc.usages & VK_IMAGE_USAGE_SAMPLED_BIT) != 0) {
        sampled_index = sampled_indices.add();

        write_descriptor_sets.push_back({
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext            = nullptr,
            .dstSet           = bindless.sets[BindlessSetType::GLOBAL],
            .dstBinding       = 1U,
            .dstArrayElement  = sampled_index.id,
            .descriptorCount  = sizeof(images_info) / sizeof(VkDescriptorImageInfo),
            .descriptorType   = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo       = images_info,
            .pBufferInfo      = nullptr,
            .pTexelBufferView = nullptr,
        });
    }

    vkUpdateDescriptorSets(device, write_descriptor_sets.size(), write_descriptor_sets.data(), 0U, nullptr);
}

void device_texture::release() {
    auto& render_device = vkdevice::get_render_device();
    auto* device = render_device.get_device();
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
    auto* device = vkdevice::get_render_device().get_device();

    VkImageViewCreateInfo create_info{
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

    for (auto mip_index{ 0U }; mip_index < mips; mip_index++) {
        create_info.subresourceRange.baseMipLevel = mip_index;
        VKCHECK(vkCreateImageView(device, &create_info, nullptr, &mips_views[mip_index]));
    }

    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount   = mips;
    VKCHECK(vkCreateImageView(device, &create_info, nullptr, &whole_view));
}


void device_surface::acquire_image_index(handle<device_semaphore> signal_handle) {
    auto& render_device = vkdevice::get_render_device();
    auto* device = render_device.get_device();
    const auto& signal = render_device.get_semaphore(signal_handle);

    vkAcquireNextImageKHR(device, vk_swapchain, -1, signal.vk_semaphore, nullptr, &image_index);
}

handle<device_texture> device_surface::get_backbuffer_image() const {
    return swapchain_images[image_index];
}
