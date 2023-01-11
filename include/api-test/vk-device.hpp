#pragma once

#include <cstdint>

#include <vulkan/vulkan_core.h>

#include "handle.hpp"
#include "freelist.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "command-buffer.hpp"

using VmaAllocator = struct VmaAllocator_T*;
using VmaAllocation = struct VmaAllocation_T*;

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
    struct queue {
        VkQueue         handle;
        uint32_t        index;
        VkQueueFlags    usages;
        VkCommandPool   command_pool;
    };

public:

    vkdevice(const window& window);

    handle<device_texture> create_texture(const texture_desc& desc);

    handle<device_buffer> create_buffer(const buffer_desc& desc);

    template<typename cmd_buf_type>
    void allocate_command_buffers(cmd_buf_type* buffers, size_t count, QueueType queue_type) {
        allocate_command_buffers(buffers, count, queue_type);
    }

    const device_texture& get_texture(handle<device_texture> handle) {
        return textures[handle.id];
    }

    const device_buffer& get_buffer(handle<device_buffer> handle) {
        return buffers[handle.id];
    }

    void submit(command_buffer* buffers, size_t count);

private:

    void allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type);

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


    queue                       queues[static_cast<uint32_t>(QueueType::MAX)];

    VkInstance                  instance;
    VkPhysicalDevice            physical_device;
    VkDevice                    device;

    VmaAllocator                gpu_allocator;

    static constexpr size_t     max_allocable_command_buffers = 16;
    static constexpr size_t     max_submitable_command_buffers = 16;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT    debug_messenger;
#endif // _DEBUG

    freelist<device_texture>    textures;
    freelist<device_buffer>     buffers;
};
