#ifndef __VULKAN_LOADER_HPP_
#define __VULKAN_LOADER_HPP_

#include <vulkan/vulkan.h>

#define VULKAN_EXPORTED_FUNCTIONS \
    X(vkGetInstanceProcAddr)      \
    X(vkGetDeviceProcAddr)


#define VULKAN_APPLICATION_FUNCTIONS          \
    X(vkCreateInstance)                       \
    X(vkEnumerateInstanceVersion)             \
    X(vkEnumerateInstanceExtensionProperties) \
    X(vkEnumerateInstanceLayerProperties)


#if defined(VK_USE_PLATFORM_WIN32_KHR)
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS X(vkCreateWin32SurfaceKHR)
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS X(vkCreateXcbSurfaceKHR)
#elif defined(VK_USE_PLATFORM_METAL_EXT)
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS X(vkCreateMetalSurfaceEXT)
#else
#define PLATFORM_VULKAN_INSTANCE_FUNCTIONS
#endif

#define VULKAN_SURFACE_FUNCTIONS                 \
    PLATFORM_VULKAN_INSTANCE_FUNCTIONS           \
    X(vkDestroySurfaceKHR)                       \
    X(vkGetPhysicalDeviceSurfaceSupportKHR)      \
    X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    X(vkGetPhysicalDeviceSurfaceFormatsKHR)      \
    X(vkGetPhysicalDeviceSurfacePresentModesKHR)

#define VULKAN_INSTANCE_FUNCTIONS                \
    VULKAN_SURFACE_FUNCTIONS                     \
    X(vkDestroyInstance)                         \
    X(vkEnumeratePhysicalDevices)                \
    X(vkGetPhysicalDeviceProperties)             \
    X(vkGetPhysicalDeviceProperties2)            \
    X(vkGetPhysicalDeviceFeatures)               \
    X(vkGetPhysicalDeviceFeatures2)              \
    X(vkGetPhysicalDeviceQueueFamilyProperties)  \
    X(vkGetPhysicalDeviceMemoryProperties)       \
    X(vkCreateDevice)                            \
    X(vkEnumerateDeviceExtensionProperties)      \
    X(vkCreateDebugUtilsMessengerEXT)            \
    X(vkDestroyDebugUtilsMessengerEXT)


#define VULKAN_SWAPCHAIN_FUNCTIONS  \
    X(vkCreateSwapchainKHR)         \
    X(vkGetSwapchainImagesKHR)      \
    X(vkAcquireNextImageKHR)        \
    X(vkDestroySwapchainKHR)        \
    X(vkQueuePresentKHR)            \

#define VULKAN_DEVICE_FUNCTIONS       \
    VULKAN_SWAPCHAIN_FUNCTIONS        \
    X(vkGetDeviceQueue)               \
    X(vkDestroyDevice)                \
    X(vkDeviceWaitIdle)               \
    X(vkQueueSubmit)                  \
    X(vkCreateSemaphore)              \
    X(vkDestroySemaphore)             \
    X(vkWaitSemaphores)               \
    X(vkCreateCommandPool)            \
    X(vkAllocateCommandBuffers)       \
    X(vkFreeCommandBuffers)           \
    X(vkDestroyCommandPool)           \
    X(vkBeginCommandBuffer)           \
    X(vkCmdPipelineBarrier)           \
    X(vkCmdPushConstants)             \
    X(vkCmdClearColorImage)           \
    X(vkCmdBeginRenderPass)           \
    X(vkCmdBindPipeline)              \
    X(vkCmdBindVertexBuffers)         \
    X(vkCmdBindIndexBuffer)           \
    X(vkCmdBindDescriptorSets)        \
    X(vkCmdSetViewport)               \
    X(vkCmdSetScissor)                \
    X(vkCmdCopyBuffer)                \
    X(vkCmdCopyImage)                 \
    X(vkCmdBlitImage)                 \
    X(vkCmdCopyBufferToImage)         \
    X(vkCmdDraw)                      \
    X(vkCmdDrawIndexed)               \
    X(vkCmdDispatch)                  \
    X(vkCmdEndRenderPass)             \
    X(vkEndCommandBuffer)             \
    X(vkResetCommandBuffer)           \
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
    X(vkGetBufferDeviceAddress)       \
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
    VULKAN_DEVICE_FUNCTIONS
#undef X


#define X(name) void Load_##name();
    VULKAN_EXPORTED_FUNCTIONS
    VULKAN_APPLICATION_FUNCTIONS
#undef X

#define X(name) void Load_##name(VkInstance _instance);
    VULKAN_INSTANCE_FUNCTIONS
#undef X

#define X(name) void Load_##name(VkDevice _device);
    VULKAN_DEVICE_FUNCTIONS
#undef X


void load_vulkan();

void load_instance_functions(VkInstance instance);

void load_device_functions(VkDevice device);

#endif // !__VULKAN_LOADER_HPP_
