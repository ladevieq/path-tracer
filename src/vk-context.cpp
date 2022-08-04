#include "vk-context.hpp"

#include <iostream>
#include <cassert>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include "vulkan-loader.hpp"

#ifdef _DEBUG
#define VKRESULT(result) assert(result == VK_SUCCESS);
#else
#define VKRESULT(result) result;
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT *data,
                                             void *userData)
{
    const char* severity_cstr = nullptr;
    switch(severity) {
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
    switch(type) {
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


vkcontext::vkcontext() {
    load_vulkan();

    create_instance();
    create_device();
    create_debugger();
    create_memory_allocator();
}

vkcontext::~vkcontext() {
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    vkDestroyInstance(instance, nullptr);
}


void vkcontext::create_instance() {
    VkApplicationInfo app_info   = {};
    app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName    = "path-tracer"; 
    app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName         = "gpu-path-tracer";
    app_info.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion          = VK_API_VERSION_1_2;

    std::vector<const char*> needed_layers = {
#if defined(_DEBUG)
        "VK_LAYER_KHRONOS_validation",
#endif
    };

    check_available_instance_layers(needed_layers);

    std::vector<const char*> needed_extensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_KHR_surface",
#if defined(LINUX)
        "VK_KHR_xcb_surface"
#elif defined(WINDOWS)
        "VK_KHR_win32_surface"
#elif defined(MACOS)
        "VK_EXT_metal_surface"
#endif
    };

    check_available_instance_extensions(needed_extensions);

    VkInstanceCreateInfo instance_create_info       = {};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &app_info;
    instance_create_info.enabledLayerCount          = needed_layers.size();
    instance_create_info.ppEnabledLayerNames        = needed_layers.data();
    instance_create_info.enabledExtensionCount      = needed_extensions.size();
    instance_create_info.ppEnabledExtensionNames    = needed_extensions.data();

    VKRESULT(vkCreateInstance(&instance_create_info, nullptr, &instance))

    load_instance_functions(instance);

    std::cerr << "Instance ready" << std::endl;
}

void vkcontext::create_debugger() {
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengeCreateInfo {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        nullptr,
        0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        DebugCallback,
        nullptr,
    };

    VKRESULT(vkCreateDebugUtilsMessengerEXT(
        instance,
        &debugUtilsMessengeCreateInfo,
        nullptr,
        &debugMessenger
    ))
}

void vkcontext::create_device() {
    select_physical_device();
    select_queue(VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);

    VkDeviceQueueCreateInfo queue_create_info   = {};
    queue_create_info.sType                     = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex          = queue_index;
    queue_create_info.queueCount                = 1;

    float queue_priority                        = 1.f;
    queue_create_info.pQueuePriorities          = &queue_priority;

    VkPhysicalDeviceVulkan12Features physical_device_12_features            = {};
    physical_device_12_features.sType                                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    physical_device_12_features.pNext                                       = VK_NULL_HANDLE;
    physical_device_12_features.bufferDeviceAddress                         = VK_TRUE;
    physical_device_12_features.runtimeDescriptorArray                      = VK_TRUE;
    physical_device_12_features.shaderStorageImageArrayNonUniformIndexing   = VK_TRUE;
    physical_device_12_features.shaderSampledImageArrayNonUniformIndexing   = VK_TRUE;
    physical_device_12_features.descriptorBindingPartiallyBound             = VK_TRUE;
    physical_device_12_features.descriptorBindingPartiallyBound             = VK_TRUE;
    physical_device_12_features.descriptorBindingUpdateUnusedWhilePending   = VK_TRUE;
    physical_device_12_features.imagelessFramebuffer                        = VK_TRUE;

    VkPhysicalDeviceFeatures2 device_features   = {};
    device_features.sType                       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    device_features.pNext                       = &physical_device_12_features;

    std::vector<const char*> device_ext         = {
        "VK_KHR_swapchain",
    };

    VkDeviceCreateInfo device_create_info       = {};
    device_create_info.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext                    = &device_features;
    device_create_info.pQueueCreateInfos        = &queue_create_info;
    device_create_info.queueCreateInfoCount     = 1;
    device_create_info.ppEnabledExtensionNames  = device_ext.data();
    device_create_info.enabledExtensionCount    = device_ext.size();
    device_create_info.enabledLayerCount        = 0; // Deprecated https://www.khronos.org/registry/vulkan/specs/1.2/html/chap31.html#extendingvulkan-layers-devicelayerdeprecation

    VKRESULT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device))

    load_device_functions(device);

    vkGetDeviceQueue(device, queue_index, 0, &queue);

    std::cerr << "Device ready" << std::endl;
}

void vkcontext::create_memory_allocator() {
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device;
    allocator_info.instance = instance;
    allocator_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

    vmaCreateAllocator(&allocator_info, &allocator);
}


void vkcontext::select_physical_device() {
    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);

    if (physical_devices_count < 1) {
        std::cerr << "No physical device found !" << std::endl;
        exit(1);
    }

    std::vector<VkPhysicalDevice> physical_devices { physical_devices_count };
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data());


    for (auto& physical_dev: physical_devices) {
        if (support_required_features(physical_dev)) {
            physical_device = physical_dev;
        }
    }

    assert(physical_device != VK_NULL_HANDLE);
}

void vkcontext::select_queue(VkQueueFlags queueUsage) {
    uint32_t queue_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties { queue_properties_count };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties.data());

    for (uint32_t properties_index = 0; properties_index < queue_properties_count; properties_index++) {
        if ((queue_properties[properties_index].queueFlags & queueUsage) != 0U) {
            queue_index = properties_index;
            return;
        }
    }

    std::cerr << "No compute queue found !" << std::endl;
    exit(1);
}

void vkcontext::check_available_instance_layers(std::vector<const char*>& needed_layers) {
    uint32_t property_count = 0;
    vkEnumerateInstanceLayerProperties(&property_count, VK_NULL_HANDLE);

    std::vector<VkLayerProperties> layer_properties { property_count };
    vkEnumerateInstanceLayerProperties(&property_count, layer_properties.data());

    uint32_t available_needed_layers_count = 0;

    for (auto& layer_property: layer_properties) {
        for (auto& needed_layer: needed_layers) {
            if (strcmp(layer_property.layerName, needed_layer) == 0) {
                available_needed_layers_count++;
            }
        }
    }

    assert(available_needed_layers_count == needed_layers.size());
}

void vkcontext::check_available_instance_extensions(std::vector<const char*>& needed_extensions) {
    uint32_t vulkan_extensions_count = 0;
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &vulkan_extensions_count, VK_NULL_HANDLE);

    std::vector<VkExtensionProperties> vulkan_extensions_properties { vulkan_extensions_count };
    vkEnumerateInstanceExtensionProperties(VK_NULL_HANDLE, &vulkan_extensions_count, vulkan_extensions_properties.data());

    uint32_t available_needed_extensions_count = 0;
    for (auto& extension: vulkan_extensions_properties) {
        for (auto& needed_extension: needed_extensions) {
            if (strcmp(extension.extensionName, needed_extension) == 0) {
                available_needed_extensions_count++;
            }
        }
    }

    assert(available_needed_extensions_count == needed_extensions.size());
}

bool vkcontext::support_required_features(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties physical_device_properties   = {};

    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);

    VkPhysicalDeviceVulkan12Features vulkan_12_features     = {};
    vulkan_12_features.sType                                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

    VkPhysicalDeviceFeatures2 physical_device_features      = {};
    physical_device_features.sType                          = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physical_device_features.pNext                          = &vulkan_12_features;

    vkGetPhysicalDeviceFeatures2(physical_device, &physical_device_features);

    if (vulkan_12_features.bufferDeviceAddress == VK_TRUE &&
        vulkan_12_features.runtimeDescriptorArray == VK_TRUE &&
        vulkan_12_features.shaderStorageImageArrayNonUniformIndexing == VK_TRUE &&
        vulkan_12_features.shaderSampledImageArrayNonUniformIndexing == VK_TRUE &&
        vulkan_12_features.descriptorBindingPartiallyBound == VK_TRUE &&
        vulkan_12_features.descriptorBindingPartiallyBound == VK_TRUE &&
        vulkan_12_features.descriptorBindingUpdateUnusedWhilePending == VK_TRUE &&
        vulkan_12_features.imagelessFramebuffer == VK_TRUE) {
        std::cerr << "Using physical device " << physical_device_properties.deviceName << std::endl;
        return true;
    }

    std::cerr << "Physical device " << physical_device_properties.deviceName << " is missing some features" << std::endl;
    return false;
}

