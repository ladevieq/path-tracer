#include <iostream>

#include "vulkan-loader.hpp"
#include "defines.hpp"

#if defined(WINDOWS)
#include <Windows.h>
#include <windowsx.h>

HMODULE vulkanLibrary = LoadLibrary("vulkan-1.dll");

#define LoadProcAddress GetProcAddress

#elif defined(LINUX)
#include <dlfcn.h>

void *vulkanLibrary = dlopen( "libvulkan.so.1", RTLD_NOW );

#define LoadProcAddress dlsym
#endif


#define X(name) PFN_##name name;
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
    VULKAN_INSTANCE_FUNCTIONS
    VULKAN_DEVICE_FUNCTIONS
#undef X

#define X(name)                                                                     \
void Load_##name()                                                              \
{                                                                               \
    name = reinterpret_cast<PFN_##name>(LoadProcAddress(vulkanLibrary, #name)); \
    if (!name) {                                                                \
        std::cerr << "Cannot load vulkan function " << #name << std::endl;      \
    }                                                                           \
}
VULKAN_EXPORTED_FUNCTIONS
#undef X

#define X(name)                                                                     \
void Load_##name()                                                              \
{                                                                               \
    name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(nullptr, #name)); \
    if (!name) {                                                                \
        std::cerr << "Cannot load vulkan function " << #name << std::endl;      \
    }                                                                           \
}
VULKAN_APPLICATION_FUNCTIONS
#undef X

#define X(name)                                                                   \
void Load_##name(VkInstance instance)                                             \
{                                                                                 \
    name = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));  \
    if (!name) {                                                                  \
        std::cerr << "Cannot load vulkan function " << #name << std::endl;        \
    }                                                                             \
}
VULKAN_INSTANCE_FUNCTIONS
#undef X

#define X(name)                                                               \
void Load_##name(VkDevice device)                                             \
{                                                                             \
    name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name)); \
    if (!name) {                                                              \
        std::cerr << "Cannot load vulkan function " << #name << std::endl;    \
    }                                                                         \
}
VULKAN_DEVICE_FUNCTIONS
#undef X

void load_vulkan()
{
#define X(name) Load_##name();
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
#undef X
};

void load_instance_functions(VkInstance instance) {
#define X(name) Load_##name(instance);
    VULKAN_INSTANCE_FUNCTIONS
#undef X
}

void load_device_functions(VkDevice device) {
#define X(name) Load_##name(device);
    VULKAN_DEVICE_FUNCTIONS
#undef X
}
