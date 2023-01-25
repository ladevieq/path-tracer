#pragma once

#include <cassert>

#include <vulkan/vulkan_core.h>

#ifdef _DEBUG
#define VKCHECK(result) assert((result) == VK_SUCCESS)
#else
#define VKCHECK(result) (result)
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                              VkDebugUtilsMessageTypeFlagsEXT             type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*);
