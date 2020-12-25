#ifndef __VULKAN_LOADER_HPP_
#define __VULKAN_LOADER_HPP_

#if defined(WINDOWS)
#include <Windows.h>
#include <windowsx.h>

#define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#define VULKAN_EXPORTED_FUNCTIONS \
    X(vkGetInstanceProcAddr)      \
    X(vkGetDeviceProcAddr)

#define VULKAN_APPLICATION_FUNCTIONS          \
    X(vkCreateInstance)                       \
    X(vkEnumerateInstanceVersion)             \
    X(vkEnumerateInstanceExtensionProperties) \
    X(vkEnumerateInstanceLayerProperties)

#define VULKAN_INSTANCE_FUNCTIONS                \
    X(vkDestroyInstance)                         \
    X(vkEnumeratePhysicalDevices)                \
    X(vkGetPhysicalDeviceProperties)             \
    X(vkGetPhysicalDeviceFeatures)               \
    X(vkGetPhysicalDeviceQueueFamilyProperties)  \
    X(vkGetPhysicalDeviceMemoryProperties)       \
    X(vkCreateDevice)                            \
    X(vkEnumerateDeviceExtensionProperties)      \
    X(vkDestroySurfaceKHR)                       \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)      \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)      \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    X(vkCreateDebugUtilsMessengerEXT)            \
    X(vkDestroyDebugUtilsMessengerEXT)

#if defined(WINDOWS)
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS X(vkCreateWin32SurfaceKHR)
#else
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS
#endif

#define VULKAN_DEVICE_FUNCTIONS       \
    X(vkGetDeviceQueue)               \
    X(vkDestroyDevice)                \
    X(vkDeviceWaitIdle)               \
    X(vkCreateSwapchainKHR)           \
    X(vkGetSwapchainImagesKHR)        \
    X(vkAcquireNextImageKHR)          \
    X(vkDestroySwapchainKHR)          \
    X(vkQueueSubmit)                  \
    X(vkQueuePresentKHR)              \
    X(vkCreateSemaphore)              \
    X(vkDestroySemaphore)             \
    X(vkCreateCommandPool)            \
    X(vkAllocateCommandBuffers)       \
    X(vkFreeCommandBuffers)           \
    X(vkDestroyCommandPool)           \
    X(vkBeginCommandBuffer)           \
    X(vkCmdPipelineBarrier)           \
    X(vkCmdClearColorImage)           \
    X(vkCmdBeginRenderPass)           \
    X(vkCmdBindPipeline)              \
    X(vkCmdBindVertexBuffers)         \
    X(vkCmdBindIndexBuffer)           \
    X(vkCmdBindDescriptorSets)        \
    X(vkCmdCopyBuffer)                \
    X(vkCmdCopyImage)                 \
    X(vkCmdCopyBufferToImage)         \
    X(vkCmdDraw)                      \
    X(vkCmdDrawIndexed)               \
    X(vkCmdEndRenderPass)             \
    X(vkEndCommandBuffer)             \
    X(vkCreateRenderPass)             \
    X(vkCreateBuffer)                 \
    X(vkCreateImage)                  \
    X(vkCreateImageView)              \
    X(vkCreateSampler)                \
    X(vkCreateFramebuffer)            \
    X(vkCreateShaderModule)           \
    X(vkCreatePipelineLayout)         \
    X(vkCreateGraphicsPipelines)      \
    X(vkCreateComputePipelines)       \
    X(vkCreateFence)                  \
    X(vkCreateDescriptorSetLayout)    \
    X(vkCreateDescriptorPool)         \
    X(vkAllocateDescriptorSets)       \
    X(vkUpdateDescriptorSets)         \
    X(vkGetBufferMemoryRequirements)  \
    X(vkGetImageMemoryRequirements)   \
    X(vkAllocateMemory)               \
    X(vkMapMemory)                    \
    X(vkInvalidateMappedMemoryRanges) \
    X(vkFlushMappedMemoryRanges)      \
    X(vkUnmapMemory)                  \
    X(vkFreeMemory)                   \
    X(vkBindBufferMemory)             \
    X(vkBindImageMemory)              \
    X(vkWaitForFences)                \
    X(vkResetFences)                  \
    X(vkDestroyFence)                 \
    X(vkFreeDescriptorSets)           \
    X(vkDestroyDescriptorPool)        \
    X(vkDestroyDescriptorSetLayout)   \
    X(vkDestroyPipeline)              \
    X(vkDestroyPipelineLayout)        \
    X(vkDestroyShaderModule)          \
    X(vkDestroyFramebuffer)           \
    X(vkDestroySampler)               \
    X(vkDestroyImageView)             \
    X(vkDestroyImage)                 \
    X(vkDestroyBuffer)                \
    X(vkDestroyRenderPass)

#define X(name) extern PFN_##name name;
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
    VULKAN_INSTANCE_FUNCTIONS
    PLATFORM_VULKAN_INSTANCE_FUNCTIONS
    VULKAN_DEVICE_FUNCTIONS
#undef X


#define X(name) void Load_##name();
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
#undef X

#define X(name) void Load_##name(VkInstance _instance);
    VULKAN_INSTANCE_FUNCTIONS
    PLATFORM_VULKAN_INSTANCE_FUNCTIONS
#undef X

#define X(name) void Load_##name(VkDevice _device);
    VULKAN_DEVICE_FUNCTIONS
#undef X

    void load_vulkan();

    void load_instance_functions(VkInstance instance);

    void load_device_functions(VkDevice device);

#endif // !__VULKAN_LOADER_HPP_
