#include "vulkan-loader.hpp"

#include <cassert>

#if defined(WINDOWS)
#include <windows.h>

HMODULE vulkanLibrary = LoadLibrary("vulkan-1.dll");

#define LoadProcAddress GetProcAddress

#elif defined(LINUX)
#include <dlfcn.h>

void *vulkanLibrary = dlopen("libvulkan.so.1", RTLD_NOW);

#define LoadProcAddress dlsym
#elif defined(MACOS)
#include <dlfcn.h>

void *vulkanLibrary = dlopen("libvulkan.1.dylib", RTLD_NOW);

#define LoadProcAddress dlsym
#endif


#define X(name) PFN_##name name;
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
    VULKAN_INSTANCE_FUNCTIONS
    VULKAN_DEVICE_FUNCTIONS
#undef X

#define X(name)                                                                     \
void Load_##name()                                                                  \
{                                                                                   \
    (name) = reinterpret_cast<PFN_##name>(LoadProcAddress(vulkanLibrary, #name));   \
    assert((name) && "Cannot load vulkan function " #name);                         \
}
VULKAN_EXPORTED_FUNCTIONS
VULKAN_APPLICATION_FUNCTIONS
#undef X

#define X(name)                                                                     \
void Load_##name(VkInstance instance)                                               \
{                                                                                   \
    (name) = reinterpret_cast<PFN_##name>(vkGetInstanceProcAddr(instance, #name));  \
    assert((name) && "Cannot load vulkan function " #name);                         \
}
VULKAN_INSTANCE_FUNCTIONS
#undef X

#define X(name)                                                                     \
void Load_##name(VkDevice device)                                                   \
{                                                                                   \
    (name) = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name));      \
    assert((name) && "Cannot load vulkan function " #name);                         \
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
