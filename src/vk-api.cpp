#include <cassert>
#include <memory>

#include "vk-api.hpp"

#define VKRESULT(result) assert(result == VK_SUCCESS);


Buffer vkapi::create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage) {
    Buffer buffer = {};

    VkBufferCreateInfo buffer_info  = {  };
    buffer_info.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size                = data_size;
    buffer_info.usage               = buffer_usage;
     
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = mem_usage;
     
    vmaCreateBuffer(context.allocator, &buffer_info, &alloc_create_info, &buffer.vkbuffer, &buffer.alloc, nullptr);

    vmaMapMemory(context.allocator, buffer.alloc, (void**) &buffer.mapped_ptr);

    return std::move(buffer);
}

void vkapi::destroy_buffer(Buffer& buffer) {
    vmaUnmapMemory(context.allocator, buffer.alloc);
    vmaDestroyBuffer(context.allocator, buffer.vkbuffer, buffer.alloc);
}
