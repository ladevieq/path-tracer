#include "vk-command-buffer.hpp"

void command_buffer::start() const {
    VkCommandBufferBeginInfo begin_info{
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VK_CHECK(vkBeginCommandBuffer(vk_command_buffer, &begin_info));

    {
        device_buffer* image_heap_buffer = vkdevice::get().get_buffer(image_heap);

        VkBindHeapInfoEXT bind_heap_info = {
            .sType               = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
            .pNext               = nullptr,
            .heapRange           = {
                .address             = image_heap_buffer->gpu_ptr,
                .size                = image_heap_buffer->size,
            },
            .reservedRangeOffset = image_heap_buffer->size - descriptor_heap_props.minResourceHeapReservedRange,
            .reservedRangeSize   = descriptor_heap_props.minResourceHeapReservedRange,
        };

        vkCmdBindResourceHeapEXT(vk_command_buffer, &bind_heap_info);
    }

    {
        device_buffer* sampler_heap_buffer = vkdevice::get().get_buffer(sampler_heap);

        VkBindHeapInfoEXT sampler_bind_heap_info = {
            .sType               = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
            .pNext               = nullptr,
            .heapRange           = {
                .address             = sampler_heap_buffer->gpu_ptr,
                .size                = sampler_heap_buffer->size,
            },
            .reservedRangeOffset = sampler_heap_buffer->size - descriptor_heap_props.minSamplerHeapReservedRange,
            .reservedRangeSize   = descriptor_heap_props.minSamplerHeapReservedRange,
        };

        vkCmdBindSamplerHeapEXT(vk_command_buffer, &sampler_bind_heap_info);
    }
}

void command_buffer::stop() const {
    VK_CHECK(vkEndCommandBuffer(vk_command_buffer));
}

void command_buffer::barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const {
    device_texture* texture = vkdevice::get().get_texture(texture_handle);

    VkImageSubresourceRange subresource_range{
        .aspectMask     = texture->aspects,
        .baseMipLevel   = 0U,
        .levelCount     = 1U,
        .baseArrayLayer = 0U,
        .layerCount     = 1U,
    };

    VkImageMemoryBarrier2 image_barrier{
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = nullptr,
        .srcStageMask        = texture->stage,
        .srcAccessMask       = texture->access,
        .dstStageMask        = stage,
        .dstAccessMask       = access,
        .oldLayout           = texture->layout,
        .newLayout           = layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = texture->vk_image,
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

    texture->layout = layout;
    texture->stage  = stage;
    texture->access = access;
}

void command_buffer::copy(handle<device_buffer> buffer_handle, ::handle<device_texture> texture_handle, uint64_t offset) const {
    const device_buffer*    buffer      = vkdevice::get().get_buffer(buffer_handle);
    const device_texture*   texture     = vkdevice::get().get_texture(texture_handle);

    VkImageSubresourceLayers image_subresource_layers{
        .aspectMask     = texture->aspects,
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
                  .width  = texture->width,
                  .height = texture->height,
                  .depth  = texture->depth,
        },
    };

    vkCmdCopyBufferToImage(vk_command_buffer, buffer->vk_buffer, texture->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &buffer_to_image_copy);
}

void graphics_command_buffer::dispatch(const dispatch_params& params, const void* data, size_t size) const {
    assert(size <= descriptor_heap_props.maxPushDataSize);

    const device_pipeline* pipeline = vkdevice::get().get_pipeline(params.pipeline);
    vkCmdBindPipeline(vk_command_buffer, pipeline->bind_point, pipeline->vk_pipeline);

    VkPushDataInfoEXT push_data {
        .sType  = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .pNext  = nullptr,
        .offset = 0,
        .data   = {
            .address    = data,
            .size       = size,
        }
    };
    vkCmdPushDataEXT(vk_command_buffer, &push_data);

    u32 x = params.group_size.vec.x / params.local_group_size.vec.x;
    u32 y = params.group_size.vec.y / params.local_group_size.vec.y;
    u32 z = params.group_size.vec.z / params.local_group_size.vec.z;
    vkCmdDispatch(vk_command_buffer, x, y, z);
}

void graphics_command_buffer::begin_renderpass(const renderpass_params& params) const {
    VkRenderingAttachmentInfo* attachments_info = static_cast<VkRenderingAttachmentInfo*>(_malloca(sizeof(VkRenderingAttachmentInfo) * params.color_attachments_count));

    for (u32 index = 0U; index < params.color_attachments_count; index++) {
        VkRenderingAttachmentInfo& attachment_info  = attachments_info[index];
        device_texture* attachment         = vkdevice::get().get_texture(params.color_attachments[index]);

        attachment_info.sType              = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        attachment_info.pNext              = nullptr;
        attachment_info.imageView          = attachment->whole_view;
        attachment_info.imageLayout        = attachment->layout;
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
        .colorAttachmentCount = static_cast<uint32_t>(params.color_attachments_count),
        .pColorAttachments    = attachments_info,
        .pDepthAttachment     = nullptr,
        .pStencilAttachment   = nullptr,
    };
    vkCmdBeginRendering(vk_command_buffer, &rendering_info);

    vkCmdSetScissor(vk_command_buffer, 0U, 1U, &scissor);
    vkCmdSetViewport(vk_command_buffer, 0U, 1U, &viewport);
}

void graphics_command_buffer::draw(const draw_params& params, const void* data, size_t size) const {
    assert(size <= descriptor_heap_props.maxPushDataSize);

    const device_pipeline* pipeline = vkdevice::get().get_pipeline(params.pipeline);
    vkCmdBindPipeline(vk_command_buffer, pipeline->bind_point, pipeline->vk_pipeline);

    VkPushDataInfoEXT push_data {
        .sType  = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .pNext  = nullptr,
        .offset = 0,
        .data   = {
            .address    = data,
            .size       = size,
        }
    };
    vkCmdPushDataEXT(vk_command_buffer, &push_data);

    vkCmdDraw(vk_command_buffer, params.vertex_count, params.instance_count, params.vertex_offset, 0U);
}

void graphics_command_buffer::draw_indexed(const draw_indexed_params& params, const void* data, size_t size) const {
    assert(size <= descriptor_heap_props.maxPushDataSize);

    const device_pipeline* pipeline = vkdevice::get().get_pipeline(params.pipeline);
    vkCmdBindPipeline(vk_command_buffer, pipeline->bind_point, pipeline->vk_pipeline);

    VkPushDataInfoEXT push_data {
        .sType  = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
        .pNext  = nullptr,
        .offset = 0,
        .data   = {
            .address    = data,
            .size       = size,
        }
    };
    vkCmdPushDataEXT(vk_command_buffer, &push_data);

    const device_buffer* index_buffer = vkdevice::get().get_buffer(params.index_buffer);
    vkCmdBindIndexBuffer(vk_command_buffer, index_buffer->vk_buffer, 0U, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(vk_command_buffer, params.vertex_count, params.instance_count, params.index_offset, static_cast<int32_t>(params.vertex_offset), 0U);
}

void graphics_command_buffer::end_renderpass() const {
    vkCmdEndRendering(vk_command_buffer);
}
