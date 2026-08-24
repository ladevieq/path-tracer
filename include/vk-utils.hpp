#pragma once

#ifdef DEBUG
#define VK_CHECK(expr)                                              \
    do {                                                            \
    VkResult res = expr;                                            \
        if (res < VK_SUCCESS) {                                     \
            printf("%s returned error code : %d", #expr, res);      \
            __debugbreak();                                         \
        }                                                           \
    }                                                               \
    while(0);
#else
#define VK_CHECK(expr) (void)expr
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                              VkDebugUtilsMessageTypeFlagsEXT             type,
                                              const VkDebugUtilsMessengerCallbackDataEXT* data,
                                              void*);
