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
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VKCHECK(vkBeginCommandBuffer(vk_command_buffer, &begin_info));

    const auto& bindless   = vkdevice::get_render_device()->get_bindingmodel();
    const auto* global_set = &bindless.sets[BindlessSetType::GLOBAL];
    if (queue_type == QueueType::GRAPHICS) {
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bindless.layout, 0, 1U, global_set, 0U, nullptr);
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, 0, 1U, global_set, 0U, nullptr);
    } else if (queue_type == QueueType::ASYNC_COMPUTE) {
        vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, 0, 1U, global_set, 0U, nullptr);
    }
}

void command_buffer::stop() const {
    VKCHECK(vkEndCommandBuffer(vk_command_buffer));
}

void command_buffer::barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const {
    auto&                   texture     = vkdevice::get_render_device()->get_texture(texture_handle);
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
    auto*                    device      = vkdevice::get_render_device();
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

void graphics_command_buffer::dispatch(const dispatch_params& params) const {
    const auto& pipeline = vkdevice::get_render_device()->get_pipeline(params.pipeline);
    const auto& bindless = vkdevice::get_render_device()->get_bindingmodel();
    vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, static_cast<uint32_t>(BindlessSetType::INSTANCES_UNIFORMS), 1U, &bindless.sets[BindlessSetType::INSTANCES_UNIFORMS], 1U, &params.uniforms_offset);
    vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, bindless.layout, static_cast<uint32_t>(BindlessSetType::DRAWS_UNIFORMS), 1U, &bindless.sets[BindlessSetType::DRAWS_UNIFORMS], 1U, &params.uniforms_offset);

    vkCmdBindPipeline(vk_command_buffer, pipeline.bind_point, pipeline.vk_pipeline);

    vkCmdDispatch(vk_command_buffer, params.group_size.x / params.local_group_size.x, params.group_size.y / params.local_group_size.y, params.group_size.z / params.local_group_size.z);
}

void graphics_command_buffer::begin_renderpass(const renderpass_params& params) const {
    std::vector<VkRenderingAttachmentInfo> attachments_info(params.color_attachments.size());

    for (auto index{ 0U }; index < params.color_attachments.size(); index++) {
        auto& attachment_info              = attachments_info[index];
        auto& attachment                   = vkdevice::get_render_device()->get_texture(params.color_attachments[index]);

        attachment_info.sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment_info.pNext              = nullptr;
        attachment_info.imageView          = attachment.whole_view;
        attachment_info.imageLayout        = attachment.layout;
        attachment_info.resolveMode        = VK_RESOLVE_MODE_NONE;
        attachment_info.resolveImageView   = nullptr;
        attachment_info.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment_info.loadOp             = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment_info.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_info.clearValue         = VkClearValue{
                    .color = {
                        .float32 = { 0.f, 0.f, 0.f, 0.f },
            },
        };
    }

    VkRect2D scissor{
        .offset{
            .x = params.render_area.x,
            .y = params.render_area.x,
        },
        .extent{
            .width  = params.render_area.width,
            .height = params.render_area.height,
        },
    };
    VkViewport viewport{
        .x        = static_cast<float>(params.render_area.x),
        .y        = static_cast<float>(params.render_area.y),
        .width    = static_cast<float>(params.render_area.width),
        .height   = static_cast<float>(params.render_area.height),
        .minDepth = 0.f,
        .maxDepth = 1.f,
    };
    VkRenderingInfo rendering_info{
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext                = nullptr,
        .flags                = 0U,
        .renderArea           = scissor,
        .layerCount           = 1U,
        .viewMask             = 0U,
        .colorAttachmentCount = static_cast<uint32_t>(attachments_info.size()),
        .pColorAttachments    = attachments_info.data(),
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr,
    };
    vkCmdBeginRendering(vk_command_buffer, &rendering_info);

    vkCmdSetScissor(vk_command_buffer, 0U, 1U, &scissor);
    vkCmdSetViewport(vk_command_buffer, 0U, 1U, &viewport);
}

void graphics_command_buffer::render(const draw_params& params) const {
    auto* device = vkdevice::get_render_device();
    const auto& pipeline = device->get_pipeline(params.pipeline);

    const auto& bindless = device->get_bindingmodel();
    vkCmdBindDescriptorSets(vk_command_buffer, pipeline.bind_point, bindless.layout, static_cast<uint32_t>(BindlessSetType::DRAWS_UNIFORMS), 1U, &bindless.sets[BindlessSetType::DRAWS_UNIFORMS], 1U, &params.uniforms_offset);

    vkCmdBindPipeline(vk_command_buffer, pipeline.bind_point, pipeline.vk_pipeline);

    if (params.index_buffer.is_valid()) {
        const auto& index_buffer = device->get_buffer(params.index_buffer);
        vkCmdBindIndexBuffer(vk_command_buffer, index_buffer.vk_buffer, 0U, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(vk_command_buffer, params.vertex_count, params.instance_count, params.index_offset, static_cast<int32_t>(params.vertex_offset), 0U);
    } else {
        vkCmdDraw(vk_command_buffer, params.vertex_count, params.instance_count, params.vertex_offset, 0U);
    }
}

void graphics_command_buffer::end_renderpass() const {
    vkCmdEndRendering(vk_command_buffer);
}
