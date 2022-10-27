#include "vk-device.hpp"
#include "color.hpp"

#include <cassert>
#include <iostream>
#include <thread>
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
                  .usages = VK_QUEUE_GRAPHICS_BIT,
              },
              // { .usages = VK_QUEUE_TRANSFER_BIT, },
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
    device_texture device_texture = {};
    device_texture.desc           = desc;

    assert(desc.mips <= texture_desc::max_mips);

    VkImageCreateInfo create_info = {};
    create_info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    create_info.pNext             = nullptr;
    create_info.flags             = 0U;
    create_info.imageType         = desc.type;
    create_info.format            = desc.format;
    create_info.extent            = {
                   .width  = desc.width,
                   .height = desc.height,
                   .depth  = desc.depth
    };
    create_info.mipLevels                     = desc.mips;
    create_info.arrayLayers                   = 1;
    create_info.samples                       = VK_SAMPLE_COUNT_1_BIT;
    create_info.tiling                        = VK_IMAGE_TILING_OPTIMAL;
    create_info.usage                         = desc.usages;
    create_info.sharingMode                   = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount         = 0;
    create_info.pQueueFamilyIndices           = nullptr;
    create_info.initialLayout                 = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.flags                   = 0U;
    alloc_create_info.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
    alloc_create_info.requiredFlags           = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    alloc_create_info.preferredFlags          = 0U;
    alloc_create_info.memoryTypeBits          = 0U;
    alloc_create_info.pool                    = nullptr;
    alloc_create_info.pUserData               = nullptr;

    VKCHECK(vmaCreateImage(gpu_allocator, &create_info, &alloc_create_info, &device_texture.handle, &device_texture.alloc, nullptr))

    if ((desc.usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0U ||
        (desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U) {
        create_views(device_texture);
    }

    return { textures.add(device_texture) };
}

handle<device_buffer> vkdevice::create_buffer(const buffer_desc& desc) {
    device_buffer device_buffer       = {};
    device_buffer.desc                = desc;

    VkBufferCreateInfo create_info    = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext                 = nullptr;
    create_info.flags                 = 0U;
    create_info.size                  = desc.size;
    create_info.usage                 = desc.usages;
    create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = nullptr;

    VmaAllocatorCreateFlags alloc_create_flags = 
        desc.memory_usage != VMA_MEMORY_USAGE_GPU_ONLY ?
        VMA_ALLOCATION_CREATE_MAPPED_BIT : 0U;
        
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.flags                   = alloc_create_flags;
    alloc_create_info.usage                   = static_cast<VmaMemoryUsage>(desc.memory_usage);
    alloc_create_info.requiredFlags           = desc.memory_properties;
    alloc_create_info.preferredFlags          = 0U;
    alloc_create_info.memoryTypeBits          = 0U;
    alloc_create_info.pool                    = nullptr;
    alloc_create_info.pUserData               = nullptr;

    VmaAllocationInfo alloc_info              = {};

    VKCHECK(vmaCreateBuffer(gpu_allocator, &create_info, &alloc_create_info, &device_buffer.handle, &device_buffer.alloc, &alloc_info))

    device_buffer.mapped_ptr = alloc_info.pMappedData;

    return { buffers.add(device_buffer) };
}

void vkdevice::submit(std::span<graphics_command_buffer> command_buffers) {
    VkSubmitInfo submit_info = {};
    submit_info.sType               = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext               = nullptr;
    submit_info.waitSemaphoreCount;
    submit_info.pWaitSemaphores;
    submit_info.pWaitDstStageMask;
    submit_info.commandBufferCount;
    submit_info.pCommandBuffers;
    submit_info.signalSemaphoreCount;
    submit_info.pSignalSemaphores;

    vkQueueSubmit()
}


// private

void vkdevice::create_views(device_texture& texture) {
    VkImageAspectFlags aspect_mask = ((texture.desc.usages & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) != 0U)
        ? VK_IMAGE_ASPECT_COLOR_BIT
        : VK_IMAGE_ASPECT_DEPTH_BIT;
    VkImageViewCreateInfo create_info = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext                 = nullptr;
    create_info.flags                 = 0U;
    create_info.image                 = texture.handle;
    create_info.viewType              = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format                = texture.desc.format;
    create_info.components            = {
                   .r = VK_COMPONENT_SWIZZLE_R,
                   .g = VK_COMPONENT_SWIZZLE_G,
                   .b = VK_COMPONENT_SWIZZLE_B,
                   .a = VK_COMPONENT_SWIZZLE_A,
    };
    create_info.subresourceRange = {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT, // TODO: Choose either color or depth based on format ?
        .baseMipLevel   = 0,
        .levelCount     = 1,
        .baseArrayLayer = 0,
        .layerCount     = 1,
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
    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = nullptr;
    create_info.flags = 0U;
    create_info.hinstance = GetModuleHandle(nullptr);
    create_info.hwnd = window.handle;

    VKCHECK(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &surface))
}

void vkdevice::create_swapchain() {
    VkBool32 supported = VK_FALSE;
    for (auto index{ 0U }; index < QueueTypes::MAX; index++) {
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

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext                   = nullptr;
    create_info.flags                   = 0U;
    create_info.surface                 = surface;
    create_info.minImageCount           = surface_capabilities.minImageCount;
    create_info.imageFormat             = supported_formats[0].format;
    create_info.imageColorSpace         = supported_formats[0].colorSpace;
    create_info.imageExtent             = extent;
    create_info.imageArrayLayers        = 1U;
    create_info.imageUsage              = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create_info.imageSharingMode        = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount   = 0U;
    create_info.pQueueFamilyIndices     = nullptr;
    create_info.preTransform            = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    create_info.compositeAlpha          = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode             = supported_present_modes[0];
    create_info.clipped                 = VK_TRUE;
    create_info.oldSwapchain            = nullptr;

    VKCHECK(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain))
}

// ----------------- vkadapter -----------------

void vkdevice::create_instance() {
    VkApplicationInfo app_info              = {};
    app_info.sType                          = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName               = "path-tracer";
    app_info.applicationVersion             = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName                    = "gpu-path-tracer";
    app_info.engineVersion                  = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion                     = VK_API_VERSION_1_3;

    constexpr const char* instance_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    constexpr const char* instance_extensions[] = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo create_info    = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pNext                   = nullptr;
    create_info.flags                   = 0U;
    create_info.pApplicationInfo        = &app_info;
    create_info.enabledLayerCount       = sizeof(instance_layers) / sizeof(instance_layers[0]);
    create_info.ppEnabledLayerNames     = instance_layers;
    create_info.enabledExtensionCount   = sizeof(instance_extensions) / sizeof(instance_extensions[0]);
    create_info.ppEnabledExtensionNames = instance_extensions;

    VKCHECK(vkCreateInstance(&create_info, nullptr, &instance))

    load_instance_functions(instance);
}

void vkdevice::create_device() {
    pick_physical_device();

    VkDeviceQueueCreateInfo queue_create_infos[QueueTypes::MAX] = {};
    float                   queue_priority                       = 1.f;

    for (auto index{ 0U }; index < QueueTypes::MAX; index++) {
        auto& queue                                = queues[index];
        auto  queue_index                          = pick_queue(queue.usages);

        queue_create_infos[index].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[index].pNext            = nullptr;
        queue_create_infos[index].flags            = 0U;
        queue_create_infos[index].queueFamilyIndex = 0;
        queue_create_infos[index].queueCount       = 1;
        queue_create_infos[index].pQueuePriorities = &queue_priority;

        queue.index                                = queue_index;
    }

    constexpr const char* device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    VkPhysicalDeviceVulkan12Features physical_device_12_features          = {};
    physical_device_12_features.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    physical_device_12_features.pNext                                     = VK_NULL_HANDLE;
    physical_device_12_features.bufferDeviceAddress                       = VK_TRUE;
    physical_device_12_features.runtimeDescriptorArray                    = VK_TRUE;
    physical_device_12_features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
    physical_device_12_features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    physical_device_12_features.descriptorBindingPartiallyBound           = VK_TRUE;
    physical_device_12_features.descriptorBindingPartiallyBound           = VK_TRUE;
    physical_device_12_features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
    physical_device_12_features.imagelessFramebuffer                      = VK_TRUE;

    VkPhysicalDeviceVulkan13Features physical_device_13_features          = {};
    physical_device_13_features.sType                                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    physical_device_13_features.pNext                                     = &physical_device_12_features;
    physical_device_13_features.synchronization2                          = VK_TRUE;
    physical_device_13_features.dynamicRendering                          = VK_TRUE;

    VkDeviceCreateInfo create_info                                        = {};
    create_info.sType                                                     = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext                                                     = &physical_device_13_features;
    create_info.flags                                                     = 0U;
    create_info.queueCreateInfoCount                                      = QueueTypes::MAX;
    create_info.pQueueCreateInfos                                         = queue_create_infos;
    create_info.enabledLayerCount                                         = 0;
    create_info.ppEnabledLayerNames                                       = nullptr;
    create_info.enabledExtensionCount                                     = sizeof(device_extensions) / sizeof(device_extensions[0]);
    create_info.ppEnabledExtensionNames                                   = device_extensions;
    create_info.pEnabledFeatures                                          = nullptr;

    VKCHECK(vkCreateDevice(physical_device, &create_info, nullptr, &device))

    load_device_functions(device);

    for (auto& queue : queues) {
        vkGetDeviceQueue(device, queue.index, 0, &queue.handle);
    }

    create_command_pools();
}

void vkdevice::create_memory_allocator() {
    VmaAllocatorCreateInfo create_info = {};
    create_info.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    create_info.physicalDevice         = physical_device;
    create_info.device                 = device;
    create_info.instance               = instance;
    create_info.vulkanApiVersion       = VK_API_VERSION_1_2;

    VKCHECK(vmaCreateAllocator(&create_info, &gpu_allocator))
}

void vkdevice::create_command_pools() {
    VkCommandPoolCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext = nullptr;
    create_info.flags = 0U;

    for (auto& queue : queues) {
        create_info.queueFamilyIndex = queue.index;

        VKCHECK(vkCreateCommandPool(device, &create_info, nullptr, &queue.command_pool))
    }
}

uint32_t vkdevice::pick_queue(VkQueueFlags queue_usages) const {
    auto queue_properties_count = 0U;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties{ queue_properties_count };
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
    VkPhysicalDeviceProperties physical_device_properties = {};

    vkGetPhysicalDeviceProperties(physical_dev, &physical_device_properties);

    // Vulkan 1.2 physical device features support
    {
        VkPhysicalDeviceVulkan12Features vulkan_12_features = {};
        vulkan_12_features.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        VkPhysicalDeviceFeatures2 physical_device_features  = {};
        physical_device_features.sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physical_device_features.pNext                      = &vulkan_12_features;

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
        VkPhysicalDeviceVulkan13Features vulkan_13_features = {};
        vulkan_13_features.sType                            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        VkPhysicalDeviceFeatures2 physical_device_features  = {};
        physical_device_features.sType                      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        physical_device_features.pNext                      = &vulkan_13_features;

        vkGetPhysicalDeviceFeatures2(physical_dev, &physical_device_features);

        if (vulkan_13_features.synchronization2 == VK_FALSE ||
            vulkan_13_features.dynamicRendering == VK_FALSE) {
            return false;
        }
    }

    return true;
}

void vkdevice::create_debug_layer_callback() {
    VkDebugUtilsMessengerCreateInfoEXT create_info = {};
    create_info.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.pNext                              = nullptr;
    create_info.flags                              = 0U;
    create_info.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    create_info.pfnUserCallback = debug_callback;
    create_info.pUserData       = nullptr;

    VKCHECK(vkCreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debug_messenger))
}
