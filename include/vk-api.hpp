#ifndef __VK_API_HPP_
#define __VK_API_HPP_

#include "vk-context.hpp"

struct Buffer {
    VmaAllocation   alloc;
    VkBuffer        vkbuffer;
    void*           mapped_ptr;
};

class vkapi {
    public:
        vkapi() = default;

        Buffer create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage);
        void destroy_buffer(Buffer& buffer);

    // private:

        vkcontext   context;

};

#endif // !__VK_API_HPP_
