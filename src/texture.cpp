#include "texture.hpp"

#include <vk_mem_alloc.h>

void Texture::create_device_image() {
    VkImageCreateInfo img_create_info = {};
    img_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext = VK_NULL_HANDLE;
    img_create_info.flags = 0;
    img_create_info.imageType = description.type;
    img_create_info.format = description.format;
    img_create_info.extent = description.size;
    img_create_info.mipLevels = 1;
    img_create_info.arrayLayers = 1;
    img_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage = description.usages;
    img_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount = 0;
    img_create_info.pQueueFamilyIndices = nullptr;
    img_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VKRESULT(vmaCreateImage(context.allocator, &img_create_info, &alloc_create_info, device_image, &image->alloc, &alloc_info))


    // image->subresource_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // image->subresource_range.baseMipLevel = 0;
    // image->subresource_range.levelCount = 1;
    // image->subresource_range.baseArrayLayer = 0;
    // image->subresource_range.layerCount = 1;

    // VkImageViewCreateInfo image_view_create_info = {};
    // image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    // image_view_create_info.pNext = nullptr;
    // image_view_create_info.flags = 0;
    // image_view_create_info.image = image->handle;
    // image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    // image_view_create_info.format = format;
    // image_view_create_info.components = {
    //     VK_COMPONENT_SWIZZLE_IDENTITY,
    //     VK_COMPONENT_SWIZZLE_IDENTITY,
    //     VK_COMPONENT_SWIZZLE_IDENTITY,
    //     VK_COMPONENT_SWIZZLE_IDENTITY
    // };
    // image_view_create_info.subresourceRange = image->subresource_range;

    // VKRESULT(vkCreateImageView(context.device, &image_view_create_info, VK_NULL_HANDLE, &image->view))

    // image->device_ptr = alloc_info.pMappedData;

    if ((usages & VK_IMAGE_USAGE_STORAGE_BIT) != 0U) {
        image->bindless_storage_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        update_descriptor_image(image_handle, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }

    if ((usages & VK_IMAGE_USAGE_SAMPLED_BIT) != 0U) {
        image->bindless_sampled_index = bindless_descriptor.allocate(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
        update_descriptor_image(image_handle, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE);
    }
}
