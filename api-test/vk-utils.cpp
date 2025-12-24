#include "vk-utils.hpp"

#include <cstdio>

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

    fprintf(stderr, "[%s_%s]:%s\n", severity_cstr, type_cstr, data->pMessage);
    return VK_FALSE;
}
