#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"
#include "freelist.hpp"
#include "texture.hpp"
#include "buffer.hpp"

using VmaAllocator = struct VmaAllocator_T*;
using VmaAllocation = struct VmaAllocation_T*;

// typedef struct VkInstance_T*                VkInstance;
// typedef struct VkDevice_T*                  VkDevice;
// typedef struct VkPhysicalDevice_T*          VkPhysicalDevice;
// typedef struct VkQueue_T*                   VkQueue;
// typedef uint32_t                            VkFlags;
// typedef VkFlags                             VkQueueUsageFlags;
// typedef struct VkImage_T*                   VkImage;
// typedef struct VkImageView_T*               VkImageView;
// typedef struct VkBuffer_T*                  VkBuffer;
// typedef enum VkImageType                    VkImageType;
// typedef enum VkFormat                       VkFormat;
// typedef VkFlags                             VkImageUsageFlags;
// 
// #ifdef _DEBUG
// typedef struct VkDebugUtilsMessengerEXT_T*  VkDebugUtilsMessengerEXT;
// #endif // _DEBUG


struct device_texture {
    VkImage         handle;
    VkImageView     whole_view;
    VkImageView     mips_views[texture_desc::max_mips] = { VK_NULL_HANDLE };
    VmaAllocation   alloc;

    texture_desc    desc;
};

struct device_buffer {
    VkBuffer        handle;
    VmaAllocation   alloc;
    VkDeviceAddress device_address;
    void*           mapped_ptr;

    buffer_desc desc;
};

class window;

class vkdevice {
public:

    vkdevice(const window& window);

    handle<device_texture> create_texture(const texture_desc& desc);

    handle<device_buffer> create_buffer(const buffer_desc& desc);

    const device_texture& get_texture(handle<device_texture> handle) {
        return textures[handle.handle];
    }

    const device_buffer& get_buffer(handle<device_buffer> handle) {
        return buffers[handle.handle];
    }

    void submit(/*std::span<command_buffer>*/);

private:

    void create_views(device_texture& texture);

    void create_surface(const window& window);

    void create_swapchain();

    VkSurfaceKHR    surface;
    VkSwapchainKHR  swapchain;

    // TODO: Wrap in a struct called adapter ?
    void create_instance();

    void create_device();

    void create_memory_allocator();

    void create_command_pools();

    void pick_physical_device();

    [[nodiscard]] uint32_t pick_queue(VkQueueFlags usages) const;

    static bool device_support_features(VkPhysicalDevice physical_dev);

    void create_debug_layer_callback();


    enum QueueTypes : uint32_t {
        GRAPHICS,
        // COPY,
        MAX,
    };

    struct queue {
        VkQueue         handle;
        uint32_t        index;
        VkQueueFlags    usages;
        VkCommandPool   command_pool;
    };

    queue               queues[QueueTypes::MAX];

    VkInstance          instance;
    VkPhysicalDevice    physical_device;
    VkDevice            device;

    VmaAllocator        gpu_allocator;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // _DEBUG

    freelist<device_texture>    textures;
    freelist<device_buffer>     buffers;
};
