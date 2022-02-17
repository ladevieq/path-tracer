#ifndef __VK_CONTEXT_HPP_
#define __VK_CONTEXT_HPP_

#include <vector>

#include "vulkan-loader.hpp"
#include "thirdparty/vk_mem_alloc.h"

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

        void select_queue();

        void check_available_instance_layers(std::vector<const char*>& needed_layers);

        void check_available_instance_extensions(std::vector<const char*>& available_layers, std::vector<const char*>& needed_extensions);

        bool support_required_features(VkPhysicalDevice physical_device);


        VkDebugUtilsMessengerEXT        debugMessenger;
};

#endif // !__VK_CONTEXT_HPP_
