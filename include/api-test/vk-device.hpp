#pragma once

#include <cstdint>
#include <span>

#include <vulkan/vulkan_core.h>

#include "command-buffer.hpp"
#include "freelist.hpp"
#include "handle.hpp"
#include "vk-bindless.hpp"
#include "vk-device-types.hpp"

using VmaAllocator = struct VmaAllocator_T*;

class window;

struct vksemaphore {
    VkSemaphore vk_semaphore = nullptr;
    uint64_t value = 0U;
};

class vkdevice {
    struct queue {
        VkQueue       vk_queue;
        uint32_t      index;
        VkQueueFlags  usages;
        VkCommandPool command_pool;
    };

    public:
    static vkdevice*        init_device(const window& window);

    static void             free_device();

    static inline vkdevice* get_render_device() {
        return render_device;
    }

    handle<device_texture>  create_texture(const texture_desc& desc);

    handle<device_buffer>   create_buffer(const buffer_desc& desc);

    handle<device_pipeline> create_pipeline(const pipeline_desc& desc);

    vksemaphore create_semaphore();

    void                    destroy_texture(const device_texture& texture);
    void                    destroy_texture(handle<device_texture> handle);

    void                    destroy_buffer(const device_buffer& buffer);
    void                    destroy_buffer(handle<device_buffer> handle);

    void                    destroy_pipeline(const device_pipeline& pipeline);
    void                    destroy_pipeline(handle<device_pipeline> handle);

    template<typename cmd_buf_type>
    void allocate_command_buffers(cmd_buf_type* buffers, size_t count, QueueType queue_type) {
        allocate_command_buffers(static_cast<command_buffer*>(buffers), count, queue_type);
    }

    inline device_texture& get_texture(handle<device_texture> handle) {
        return textures[handle.id];
    }

    inline const device_buffer& get_buffer(handle<device_buffer> handle) {
        return buffers[handle.id];
    }

    inline const device_pipeline& get_pipeline(handle<device_pipeline> handle) {
        return pipelines[handle.id];
    }

    inline const bindless_model& get_bindingmodel() {
        return bindless;
    }

    inline VkDevice get_device() {
        return device;
    }

    void wait(vksemaphore& semaphore);

    void submit(std::span<command_buffer> buffers, vksemaphore* wait, vksemaphore* signal);

    void present(device_surface& surface, const vksemaphore& semaphore);

    private:
    vkdevice(const window& window);
    ~vkdevice();

    void allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type);

    void create_views(device_texture& texture);

    // Presentation stuff
    void           create_surface(const window& window);

    void           create_swapchain();

    void           create_virtual_frames();

    VkSurfaceKHR   surface   = nullptr;
    VkSwapchainKHR swapchain = nullptr;

    // TODO: Wrap in a struct called adapter ?
    void        create_instance();

    void        create_device();

    void        create_memory_allocator();

    void        create_command_pools();

    void        pick_physical_device();

    static bool device_support_features(VkPhysicalDevice physical_dev);

    void        create_debug_layer_callback();

    // Virtual frames resources
    static constexpr uint32_t virtual_frames_count = 3U;
    VkSemaphore               semaphores[virtual_frames_count];

    queue                     queues[static_cast<uint32_t>(QueueType::MAX)];

    VkInstance                instance;
    VkPhysicalDevice          physical_device;
    VkDevice                  device;

    VmaAllocator              gpu_allocator;

    bindless_model            bindless;

    static constexpr size_t   max_allocable_command_buffers  = 16;
    static constexpr size_t   max_submitable_command_buffers = 16;

    freelist<device_texture*> storage_indices;
    freelist<device_texture*> sampled_indices;

    freelist<device_texture>  textures;
    freelist<device_buffer>   buffers;
    freelist<device_pipeline> pipelines;

    static vkdevice*          render_device;

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debug_messenger;
#endif // _DEBUG
};
