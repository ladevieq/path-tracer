#ifndef __VK_CONTEXT_HPP_
#define __VK_CONTEXT_HPP_

#include <vector>
#include "vulkan-loader.hpp"

using VmaAllocator = struct VmaAllocator_T*;

class vkcontext {
    public:
        vkcontext();

        ~vkcontext();

        VkInstance          instance;

        VkPhysicalDevice    physical_device;
        VkDevice            device;

        VmaAllocator        allocator;

        uint32_t            queue_index;
        VkQueue             queue;

    private:
        void create_instance();

        void create_debugger();

        void create_device();

        void create_memory_allocator();

        void select_physical_device();

        void select_queue(VkQueueFlags queueUsage);

        static void check_available_instance_layers(std::vector<const char*>& needed_layers);

        static void check_available_instance_extensions(std::vector<const char*>& needed_extensions);

        static bool support_required_features(VkPhysicalDevice physical_device);


        VkDebugUtilsMessengerEXT        debugMessenger;
};

#endif // !__VK_CONTEXT_HPP_
