#include "vk-device.hpp"

#include <cassert>
#include <iostream>
#include <vector>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vulkan-loader.hpp"
#include "window.hpp"

#ifdef _DEBUG
#define VKCHECK(result) assert(result == VK_SUCCESS);
#else
#define VKCHECK(result) result;
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                              VkDebugUtilsMessageTypeFlagsEXT             type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*) {
    const char* severity_cstr = nullptr;
    switch (severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        severity_cstr = "VERBOSE";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        severity_cstr = "INFO";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        severity_cstr = "WARNING";
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        severity_cstr = "ERROR";
        break;
    default:
        break;
    }

    const char* type_cstr = nullptr;
    switch (type) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        type_cstr = "GENERAL";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        type_cstr = "VALIDATION";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
        type_cstr = "PERFORMANCE";
        break;
    default:
        break;
    }

    std::cerr << "[" << severity_cstr << "_" << type_cstr << "]: " << data->pMessage << std::endl;
    return VK_FALSE;
}

vkdevice::vkdevice(const window& window)
        : queues{
        {
            .usages = VK_QUEUE_GRAPHICS_BIT |
                      VK_QUEUE_COMPUTE_BIT  |
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

#ifdef _DEBUG
    create_debug_layer_callback();
#endif // _DEBUG

    create_device();

    create_memory_allocator();

    create_surface(window);

    create_swapchain();
}

handle<device_texture> vkdevice::create_texture(const texture_desc& desc) {
    device_texture device_texture {
        .desc = desc,
    };

    assert(desc.mips <= texture_desc::max_mips);

    VkImageCreateInfo create_info {
        .sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0U,
        .imageType             = desc.type,
        .format                = desc.format,
        .extent                = {
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

    VmaAllocationCreateInfo alloc_create_info {
        .flags                   = 0U,
        .usage                   = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .preferredFlags          = 0U,
        .memoryTypeBits          = 0U,
        .pool                    = nullptr,
        .pUserData               = nullptr,
    };

    VKCHECK(vmaCreateImage(gpu_allocator, &create_info, &alloc_create_info, &device_texture.handle, &device_texture.alloc, nullptr))

    if ((desc.usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0U ||
        (desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U) {
        create_views(device_texture);
    }

    return { textures.add(device_texture) };
}

handle<device_buffer> vkdevice::create_buffer(const buffer_desc& desc) {
    device_buffer device_buffer {
        .desc = desc,
    };

    VkBufferCreateInfo create_info {
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
        desc.memory_usage != VMA_MEMORY_USAGE_GPU_ONLY ?
        VMA_ALLOCATION_CREATE_MAPPED_BIT : 0U;

    VmaAllocationCreateInfo alloc_create_info {
        .flags          = alloc_create_flags,
        .usage          = static_cast<VmaMemoryUsage>(desc.memory_usage),
        .requiredFlags  = desc.memory_properties,
        .preferredFlags = 0U,
        .memoryTypeBits = 0U,
        .pool           = nullptr,
        .pUserData      = nullptr,
    };

    VmaAllocationInfo alloc_info {};

    VKCHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.handle, &device_buffer.alloc, &alloc_info))

    device_buffer.mapped_ptr = alloc_info.pMappedData;

    return { buffers.add(device_buffer) };
}

void vkdevice::submit(command_buffer* buffers, size_t count) {
    assert((count - max_submitable_command_buffers) <= 0);
    const auto queue_type = buffers[0].queue_type;
    VkCommandBuffer to_submit[max_submitable_command_buffers];

    for (auto index { 0U }; index < count; index++) {
        assert(queue_type == buffers[index].queue_type);
        to_submit[index] = buffers[index].handle;
    }

    VkSubmitInfo submit_info {
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

    VKCHECK(vkQueueSubmit(queues[static_cast<uint32_t>(queue_type)].handle, 1, &submit_info, nullptr))
}


// private
void vkdevice::allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type) {
    assert((count - max_allocable_command_buffers) <= 0);
    const auto& queue = queues[static_cast<uint32_t>(type)];
    VkCommandBuffer command_buffers[max_allocable_command_buffers];

    VkCommandBufferAllocateInfo allocate_info {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = queue.command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = static_cast<uint32_t>(count),
    };

    VKCHECK(vkAllocateCommandBuffers(device, &allocate_info, command_buffers))

    for (auto index { 0U }; index < count; index++) {
        buffers[index].handle = command_buffers[index];
    }
}

void vkdevice::create_views(device_texture& texture) {
    VkImageAspectFlags aspect_mask = ((texture.desc.usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0U)
        ? VK_IMAGE_ASPECT_COLOR_BIT
        : VK_IMAGE_ASPECT_DEPTH_BIT;
    VkImageViewCreateInfo create_info {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0U,
        .image            = texture.handle,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = texture.desc.format,
        .components       = {
           .r = VK_COMPONENT_SWIZZLE_R,
           .g = VK_COMPONENT_SWIZZLE_G,
           .b = VK_COMPONENT_SWIZZLE_B,
           .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Choose either color or depth based on format ?
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    for (auto mip_index{ 0 }; mip_index < texture.desc.mips; mip_index++) {
        create_info.subresourceRange.baseMipLevel = mip_index;
        VKCHECK(vkCreateImageView(device, &create_info, nullptr, &texture.mips_views[mip_index]))
    }

    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount   = texture.desc.mips;
    VKCHECK(vkCreateImageView(device, &create_info, nullptr, &texture.whole_view))
}

void vkdevice::create_surface(const window& window) {
    VkWin32SurfaceCreateInfoKHR create_info {
        .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .pNext      = nullptr,
        .flags      = 0U,
        .hinstance  = GetModuleHandle(nullptr),
        .hwnd       = window.handle,
    };

    VKCHECK(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface))
}

void vkdevice::create_swapchain() {
    VkBool32 supported = VK_FALSE;
    for (auto index{ 0U }; index < static_cast<uint32_t>(QueueType::MAX); index++) {
        auto& queue = queues[index];
        VKCHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue.index, surface, &supported))

        assert(supported);
    }

    VkSurfaceCapabilitiesKHR surface_capabilities = {};
    VKCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities))

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
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_formats_count, VK_NULL_HANDLE))

    std::vector<VkSurfaceFormatKHR> supported_formats{ supported_formats_count };
    VKCHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &supported_formats_count, supported_formats.data()))


    auto supported_present_modes_count = 0U;
    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_present_modes_count, VK_NULL_HANDLE))

    std::vector<VkPresentModeKHR> supported_present_modes{ supported_present_modes_count };

    VKCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &supported_present_modes_count, supported_present_modes.data()))

    VkSwapchainCreateInfoKHR create_info = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext                 = nullptr,
        .flags                 = 0U,
        .surface               = surface,
        .minImageCount         = surface_capabilities.minImageCount,
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

    VKCHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain))
}

// ----------------- vkadapter -----------------

void vkdevice::create_instance() {
    VkApplicationInfo app_info {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName   = "path-tracer",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "gpu-path-tracer",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_3,
    };

    constexpr const char* instance_layers[] {
        "VK_LAYER_KHRONOS_validation",
    };

    constexpr const char* instance_extensions[] {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo create_info {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0U,
        .pApplicationInfo        = &app_info,
        .enabledLayerCount       = sizeof(instance_layers) / sizeof(instance_layers[0]),
        .ppEnabledLayerNames     = instance_layers,
        .enabledExtensionCount   = sizeof(instance_extensions) / sizeof(instance_extensions[0]),
        .ppEnabledExtensionNames = instance_extensions,
    };

    VKCHECK(vkCreateInstance(&create_info, nullptr, &instance))

    load_instance_functions(instance);
}

void vkdevice::create_device() {
    pick_physical_device();

    float                   queue_priority = 1.f;
    VkDeviceQueueCreateInfo queue_create_infos[static_cast<uint32_t>(QueueType::MAX)] {
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        nullptr,
        0U,
        0,
        1,
        &queue_priority,
    };

    for (auto index{ 0U }; index < static_cast<uint32_t>(QueueType::MAX); index++) {
        auto& queue       = queues[index];
        auto  queue_index = pick_queue(queue.usages);

        queue.index       = queue_index;
    }

    constexpr const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan12Features physical_device_12_features {
        .sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext                                     = VK_NULL_HANDLE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
        .descriptorBindingPartiallyBound           = VK_TRUE,
        .runtimeDescriptorArray                    = VK_TRUE,
        .imagelessFramebuffer                      = VK_TRUE,
        .bufferDeviceAddress                       = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features physical_device_13_features {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &physical_device_12_features,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkDeviceCreateInfo create_info {
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

    VKCHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device))

    load_device_functions(device);

    for (auto& queue : queues) {
        vkGetDeviceQueue(device, queue.index, 0, &queue.handle);
    }

    create_command_pools();
}

void vkdevice::create_memory_allocator() {
    VmaAllocatorCreateInfo create_info {
        .flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice         = physical_device,
        .device                 = device,
        .instance               = instance,
        .vulkanApiVersion       = VK_API_VERSION_1_2,
    };

    VKCHECK(vmaCreateAllocator(&create_info, &gpu_allocator))
}

void vkdevice::create_command_pools() {
    VkCommandPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0U,
    };

    for (auto& queue : queues) {
        create_info.queueFamilyIndex = queue.index;

        VKCHECK(vkCreateCommandPool(device, &create_info, nullptr, &queue.command_pool))
    }
}

uint32_t vkdevice::pick_queue(VkQueueFlags queue_usages) const {
    auto queue_properties_count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties { queue_properties_count };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties.data());

    for (auto properties_index{ 0U }; properties_index < queue_properties_count; properties_index++) {
        if ((queue_properties[properties_index].queueFlags & queue_usages) == queue_usages) {
            return properties_index;
        }
    }

    assert(true);
    return UINT_MAX;
}

void vkdevice::pick_physical_device() {
    auto physical_devices_count = 0U;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);

    assert(physical_devices_count > 0);

    std::vector<VkPhysicalDevice> physical_devices { physical_devices_count };
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
        VkPhysicalDeviceVulkan12Features vulkan_12_features {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        };

        VkPhysicalDeviceFeatures2 physical_device_features {
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
            vulkan_12_features.descriptorBindingPartiallyBound              == VK_FALSE ||
            vulkan_12_features.descriptorBindingUpdateUnusedWhilePending    == VK_FALSE ||
            vulkan_12_features.imagelessFramebuffer                         == VK_FALSE) {
            return false;
        }
        // clang-format on
    }

    // Vulkan 1.3 physical device features support
    {
        VkPhysicalDeviceVulkan13Features vulkan_13_features {
            .sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        };

        VkPhysicalDeviceFeatures2 physical_device_features {
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
    VkDebugUtilsMessengerCreateInfoEXT create_info {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0U,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData       = nullptr,
    };

    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger))
}
