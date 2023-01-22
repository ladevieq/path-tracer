#include "command-buffer.hpp"

#include <cassert>
#include <vulkan/vulkan_core.h>

#include "vulkan-loader.hpp"

#include "vk-device.hpp"
#include "vk-utils.hpp"

void command_buffer::start() const {
    VkCommandBufferBeginInfo begin_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = 0U,
        .pInheritanceInfo = nullptr
    };

    VKCHECK(vkBeginCommandBuffer(vk_command_buffer, &begin_info));

    const auto& bindless = vkdevice::get_device()->get_bindingmodel();
    if (queue_type == QueueType::GRAPHICS) {
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindless.layout, 0, BindlessSetType::MAX, bindless.sets, 0, nullptr);
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, 0, BindlessSetType::MAX, bindless.sets, 0, nullptr);
    } else if (queue_type == QueueType::ASYNC_COMPUTE) {
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, 0, BindlessSetType::MAX, bindless.sets, 0, nullptr);
    }
}

void command_buffer::stop() const {
    VKCHECK(vkEndCommandBuffer(vk_command_buffer));
}

void command_buffer::barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const {
    auto&                   texture     = vkdevice::get_device()->get_texture(texture_handle);
    VkImageAspectFlags      aspect_mask = ((texture.desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
                                              ? VK_IMAGE_ASPECT_DEPTH_BIT
                                              : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageSubresourceRange subresource_range{
        .aspectMask     = aspect_mask,
        .baseMipLevel   = 0U,
        .levelCount     = 1U,
        .baseArrayLayer = 0U,
        .layerCount     = 1U,
    };

    VkImageMemoryBarrier2 image_barrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = texture.stage,
        .srcAccessMask       = texture.access,
        .dstStageMask        = stage,
        .dstAccessMask       = access,
        .oldLayout           = texture.layout,
        .newLayout           = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = texture.vk_image,
        .subresourceRange    = subresource_range,
    };

    VkDependencyInfo dependency_info{
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                    = nullptr,
        .dependencyFlags          = VK_DEPENDENCY_BY_REGION_BIT,
        .memoryBarrierCount       = 0U,
        .pMemoryBarriers          = nullptr,
        .bufferMemoryBarrierCount = 0U,
        .pBufferMemoryBarriers    = nullptr,
        .imageMemoryBarrierCount  = 1U,
        .pImageMemoryBarriers     = &image_barrier,
    };
    vkCmdPipelineBarrier2(vk_command_buffer, &dependency_info);

    texture.layout = layout;
    texture.stage  = stage;
    texture.access = access;
}

void command_buffer::copy(handle<device_buffer> buffer_handle, ::handle<device_texture> texture_handle, VkDeviceSize offset) const {
    auto* device = vkdevice::get_device();
    const auto&              buffer      = device->get_buffer(buffer_handle);
    const auto&              texture     = device->get_texture(texture_handle);
    VkImageAspectFlags       aspect_mask = ((texture.desc.usages & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U)
                                               ? VK_IMAGE_ASPECT_DEPTH_BIT
                                               : VK_IMAGE_ASPECT_COLOR_BIT;
    VkImageSubresourceLayers image_subresource_layers{
        .aspectMask     = aspect_mask,
        .mipLevel       = 0,
        .baseArrayLayer = 0,
        .layerCount     = 1,
    };

    VkBufferImageCopy buffer_to_image_copy{
        .bufferOffset      = offset,
        .bufferRowLength   = 0,
        .bufferImageHeight = 0,
        .imageSubresource  = image_subresource_layers,
        .imageOffset       = { .x = 0, .y = 0, .z = 0 },
        .imageExtent       = {
                  .width  = texture.desc.width,
                  .height = texture.desc.height,
                  .depth  = texture.desc.depth,
        },
    };

    vkCmdCopyBufferToImage(vk_command_buffer, buffer.vk_buffer, texture.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_to_image_copy);
}

void graphics_command_buffer::dispatch(const dispatch_params& params) {
    const auto& pipeline = vkdevice::get_device()->get_pipeline(params.pipeline);

    vkCmdBindPipeline(vk_command_buffer, pipeline.bind_point, pipeline.vk_pipeline);

    vkCmdDispatch(vk_command_buffer, params.group_size.x / params.local_group_size.x, params.group_size.y / params.local_group_size.y, params.group_size.z / params.local_group_size.z);
}
