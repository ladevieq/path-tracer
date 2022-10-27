#include "vk-command-buffer.hpp"

#include <cassert>

#include "vulkan-loader.hpp"

#ifdef _DEBUG
#define VKCHECK(result) assert(result == VK_SUCCESS);
#else
#define VKCHECK(result) result;
#endif

void command_buffer::start() {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = 0U;
    begin_info.pInheritanceInfo = nullptr;

    VKCHECK(vkBeginCommandBuffer(handle, &begin_info))
}

void command_buffer::stop() {
    VKCHECK(vkEndCommandBuffer(handle))
}
