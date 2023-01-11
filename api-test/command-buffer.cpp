#include "command-buffer.hpp"

#include <cassert>

#include "vulkan-loader.hpp"

#include"vk-device.hpp"

#ifdef _DEBUG
#define VKCHECK(result) assert(result == VK_SUCCESS);
#else
#define VKCHECK(result) result;
#endif

void command_buffer::start() const {
    VkCommandBufferBeginInfo begin_info {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext              = nullptr,
        .flags              = 0U,
        .pInheritanceInfo   = nullptr
    };

    VKCHECK(vkBeginCommandBuffer(handle, &begin_info))
}

void command_buffer::stop() const {
    VKCHECK(vkEndCommandBuffer(handle))
}

void transfer_command_buffer::copy(::handle<device_buffer> buffer_handle, ::handle<device_texture> texture_handle) {
    const auto& buffer = vkdevice::get_buffer(buffer_handle);
    const auto& texture = vkdevice::get_texture(texture_handle);
    VkImageSubresourceLayers image_subresource_layers {
        .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    VkBufferImageCopy buffer_to_image_copy {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,
        .imageSubresource   = image_subresource_layers,
        .imageOffset        = { .x = 0, .y = 0, .z = 0 },
        .imageExtent        = {
            .width  = texture.desc.width,
            .height = texture.desc.height,
            .depth  = texture.desc.depth,
        },
    };

    vkCmdCopyBufferToImage(handle, buffer.handle, texture.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_to_image_copy);
}
