#pragma once

#include <vulkan/vulkan_core.h>

using VmaAllocator = struct VmaAllocator_T*;

struct queue {
    VkQueue         handle;
    uint32_t        index;
    VkQueueFlags    usages;
};

class vkcontext {
    public:
        vkcontext();

        ~vkcontext();

        VkInstance          instance;

        VkPhysicalDevice    physical_device;
        VkDevice            device;

        VmaAllocator        allocator;

        queue               graphics_queue;

    private:
        void create_instance();

        void create_debugger();

        void create_device();

        void create_memory_allocator();

        void select_physical_device();

        [[nodiscard]] uint32_t select_queue(VkQueueFlags queue_usage) const;

        static void check_available_instance_layers(const char* needed_layers[], size_t needed_layers_count);

        static void check_available_instance_extensions(const char* needed_extensions[], size_t needed_extensions_count);

        static bool support_required_features(VkPhysicalDevice physical_device);


        VkDebugUtilsMessengerEXT        debugMessenger;
};
