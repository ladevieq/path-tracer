#include <cassert>
#include <memory>

#include "vk-api.hpp"
#include "window.hpp"
#include "utils.hpp"

#define VKRESULT(result) assert(result == VK_SUCCESS);


vkapi::vkapi() {
    VkCommandPoolCreateInfo cmd_pool_create_info    = {};
    cmd_pool_create_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext                      = nullptr;
    cmd_pool_create_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex           = context.queue_index;

    VKRESULT(vkCreateCommandPool(context.device, &cmd_pool_create_info, nullptr, &command_pool))


    std::vector<VkDescriptorPoolSize> descriptor_pools_sizes = {
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount                    = (uint32_t)0xff,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount                    = (uint32_t)0xff,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info  = {};
    descriptor_pool_create_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext                       = nullptr;
    descriptor_pool_create_info.flags                       = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets                     = 0xff;
    descriptor_pool_create_info.poolSizeCount               = descriptor_pools_sizes.size();
    descriptor_pool_create_info.pPoolSizes                  = descriptor_pools_sizes.data();

    VKRESULT(vkCreateDescriptorPool(context.device, &descriptor_pool_create_info, nullptr, &descriptor_pool))
}

vkapi::~vkapi() {
    vkDestroyCommandPool(context.device, command_pool, nullptr);
    vkDestroyDescriptorPool(context.device, descriptor_pool, nullptr);
}


Buffer vkapi::create_buffer(size_t data_size, VkBufferUsageFlags buffer_usage, VmaMemoryUsage mem_usage) {
    Buffer buffer = {};

    VkBufferCreateInfo buffer_info  = {  };
    buffer_info.sType               = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size                = data_size;
    buffer_info.usage               = buffer_usage;
     
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = mem_usage;
     
    vmaCreateBuffer(context.allocator, &buffer_info, &alloc_create_info, &buffer.handle, &buffer.alloc, nullptr);

    vmaMapMemory(context.allocator, buffer.alloc, (void**) &buffer.mapped_ptr);

    return std::move(buffer);
}

void vkapi::destroy_buffer(Buffer& buffer) {
    vmaUnmapMemory(context.allocator, buffer.alloc);
    vmaDestroyBuffer(context.allocator, buffer.handle, buffer.alloc);
}


Image vkapi::create_image(VkExtent3D size, VkFormat format, VkImageUsageFlags usages) {
    Image image;

    image.subresource_range.aspectMask      = VK_IMAGE_ASPECT_COLOR_BIT;
    image.subresource_range.baseMipLevel    = 0;
    image.subresource_range.levelCount      = 1;
    image.subresource_range.baseArrayLayer  = 0;
    image.subresource_range.layerCount      = 1;

    VkImageCreateInfo img_create_info       = {};
    img_create_info.sType                   = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_create_info.pNext                   = VK_NULL_HANDLE;
    img_create_info.flags                   = 0;
    img_create_info.imageType               = VK_IMAGE_TYPE_2D;
    img_create_info.format                  = format;
    img_create_info.extent                  = size;
    img_create_info.mipLevels               = 1;
    img_create_info.arrayLayers             = 1;
    img_create_info.samples                 = VK_SAMPLE_COUNT_1_BIT;
    img_create_info.tiling                  = VK_IMAGE_TILING_OPTIMAL;
    img_create_info.usage                   = usages;
    img_create_info.sharingMode             = VK_SHARING_MODE_EXCLUSIVE;
    img_create_info.queueFamilyIndexCount   = 0;
    img_create_info.pQueueFamilyIndices     = nullptr;
    img_create_info.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;

    vkCreateImage(context.device, &img_create_info, VK_NULL_HANDLE, &image.handle); 

    VkImageViewCreateInfo image_view_create_info    = {};
    image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext                    = nullptr;
    image_view_create_info.flags                    = 0;
    image_view_create_info.image                    = image.handle;
    image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                   = format;
    image_view_create_info.components               = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY
    };
    image_view_create_info.subresourceRange        = image.subresource_range;

    vkCreateImage(context.device, &img_create_info, VK_NULL_HANDLE, &image.handle); 

    return std::move(image);
}
void vkapi::destroy_image(Image &image) {
    vkDestroyImage(context.device, image.handle, nullptr);
    vkDestroyImageView(context.device, image.view, nullptr);
}

std::vector<Image> vkapi::create_images(VkExtent3D size, VkFormat format, VkImageUsageFlags usages, size_t image_count) {
    std::vector<Image> images { image_count };

    for (size_t image_index = 0; image_index < image_count; image_index++) {
        images[image_index] = create_image(size, format, usages);
    }

    return std::move(images);
}

void vkapi::destroy_images(std::vector<Image> &images) {
    for (auto &image: images) {
        destroy_image(image);
    }
}


VkFence vkapi::create_fence() {
    VkFence fence;
    VkFenceCreateInfo create_info   = {};
    create_info.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.pNext               = nullptr;
    create_info.flags               = VK_FENCE_CREATE_SIGNALED_BIT;

    VKRESULT(vkCreateFence(context.device, &create_info, nullptr, &fence))

    return fence;
}

void vkapi::destroy_fence(VkFence fence) {
    vkDestroyFence(context.device, fence, nullptr);
}

std::vector<VkFence> vkapi::create_fences(size_t fences_count) {
    std::vector<VkFence> fences { fences_count };

    for(size_t fence_index = 0; fence_index < fences_count; fence_index++) {
        fences[fence_index] = create_fence();
    }

    return std::move(fences);
}

void vkapi::destroy_fences(std::vector<VkFence> &fences) {
    for(auto &fence: fences) {
        vkDestroyFence(context.device, fence, nullptr);
    }
}


VkSemaphore vkapi::create_semaphore() {
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo create_info   = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext                   = nullptr;
    create_info.flags                   = 0;

    VKRESULT(vkCreateSemaphore(context.device, &create_info, nullptr, &semaphore))

    return semaphore;
}

void vkapi::destroy_semaphore(VkSemaphore semaphore) {
    vkDestroySemaphore(context.device, semaphore, nullptr);
}

std::vector<VkSemaphore> vkapi::create_semaphores(size_t semaphores_count) {
    std::vector<VkSemaphore> semaphores { semaphores_count };

    for (size_t semaphore_index = 0; semaphore_index < semaphores_count; semaphore_index++) {
        semaphores[semaphore_index] = create_semaphore();
    }

    return std::move(semaphores);
}

void vkapi::destroy_semaphores(std::vector<VkSemaphore> &semaphores) {
    for (auto &semaphore: semaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }
}


std::vector<VkCommandBuffer> vkapi::create_command_buffers(size_t command_buffers_count) {
    VkCommandBufferAllocateInfo cmd_buf_allocate_info   = {};
    cmd_buf_allocate_info.sType                         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_allocate_info.pNext                         = nullptr;
    cmd_buf_allocate_info.commandPool                   = command_pool;
    cmd_buf_allocate_info.level                         = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_allocate_info.commandBufferCount            = command_buffers_count;

    std::vector<VkCommandBuffer> command_buffers { command_buffers_count };
    vkAllocateCommandBuffers(context.device, &cmd_buf_allocate_info, command_buffers.data());

    return std::move(command_buffers);
}

void vkapi::destroy_command_buffers(std::vector<VkCommandBuffer> &command_buffers) {
    vkFreeCommandBuffers(context.device, command_pool, command_buffers.size(), command_buffers.data());
}


ComputePipeline vkapi::create_compute_pipeline(const char* shader_path, std::vector<VkDescriptorSetLayoutBinding> bindings) {
    ComputePipeline pipeline;

    std::vector<uint8_t> shader_code = get_shader_code(shader_path);
    if (shader_code.size() == 0) {
        exit(1);
    }

    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = nullptr;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = shader_code.size();
    shader_create_info.pCode                        = (uint32_t*)shader_code.data();

    VKRESULT(vkCreateShaderModule(context.device, &shader_create_info, nullptr, &pipeline.shader_module))

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = nullptr;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = pipeline.shader_module;
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = nullptr;


    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = nullptr;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = bindings.size();
    descriptor_set_layout_create_info.pBindings     = bindings.data();

    VKRESULT(vkCreateDescriptorSetLayout(context.device, &descriptor_set_layout_create_info, nullptr, &pipeline.descriptor_set_layout))

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &pipeline.descriptor_set_layout;
    layout_create_info.pushConstantRangeCount       = 0;
    layout_create_info.pPushConstantRanges          = nullptr;

    VKRESULT(vkCreatePipelineLayout(context.device, &layout_create_info, nullptr, &pipeline.layout))

    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = nullptr;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = pipeline.layout;

    VKRESULT(vkCreateComputePipelines(
        context.device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &pipeline.handle
    ))

    return std::move(pipeline);
}

void vkapi::destroy_compute_pipeline(ComputePipeline &pipeline) {
    vkDestroyShaderModule(context.device, pipeline.shader_module, nullptr);
    vkDestroyDescriptorSetLayout(context.device, pipeline.descriptor_set_layout, nullptr);
    vkDestroyPipelineLayout(context.device, pipeline.layout, nullptr);
    vkDestroyPipeline(context.device, pipeline.handle, nullptr);
}


std::vector<VkDescriptorSet> vkapi::create_descriptor_sets(VkDescriptorSetLayout descriptor_sets_layout, size_t descriptor_sets_count) {
    std::vector<VkDescriptorSetLayout> sets_layouts = { descriptor_sets_count, descriptor_sets_layout };

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info    = {};
    descriptor_set_allocate_info.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext                          = nullptr;
    descriptor_set_allocate_info.descriptorPool                 = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount             = sets_layouts.size();
    descriptor_set_allocate_info.pSetLayouts                    = sets_layouts.data();

    std::vector<VkDescriptorSet> descriptor_sets { descriptor_sets_count };
    vkAllocateDescriptorSets(context.device, &descriptor_set_allocate_info, descriptor_sets.data());

    return std::move(descriptor_sets);
}

void vkapi::destroy_descriptor_sets(std::vector<VkDescriptorSet> &descriptor_sets) {
    vkFreeDescriptorSets(context.device, descriptor_pool, descriptor_sets.size(), descriptor_sets.data());
}


VkSurfaceKHR vkapi::create_surface(window& wnd) {
    VkSurfaceKHR surface;

#if defined(LINUX)
    VkXcbSurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.connection                  = wnd.connection;
    create_info.window                      = wnd.win;

    VKRESULT(vkCreateXcbSurfaceKHR(context.instance, &create_info, nullptr, &surface))
#elif defined(WINDOWS)
    VkWin32SurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.hinstance                   = GetModuleHandle(NULL);
    create_info.hwnd                        = wnd.win_handle;

    VKRESULT(vkCreateWin32SurfaceKHR(instance, &create_info, nullptr, &platform_surface))
#endif

    return surface;
}

void vkapi::destroy_surface(VkSurfaceKHR surface) {
    vkDestroySurfaceKHR(context.instance, surface, nullptr);
}


Swapchain vkapi::create_swapchain(VkSurfaceKHR surface, size_t min_image_count, VkImageUsageFlags usages, std::optional<Swapchain> old_swapchain) {
    Swapchain swapchain;
    VkBool32 queue_support_presentation = VK_FALSE;
    VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(context.physical_device, context.queue_index, surface, &queue_support_presentation))

    assert(queue_support_presentation == true);

    VkSurfaceCapabilitiesKHR caps;
    VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context.physical_device, surface, &caps))

    VkExtent2D wantedExtent = caps.currentExtent;

    if (wantedExtent.width < caps.minImageExtent.width) {
        wantedExtent.width = caps.minImageExtent.width;
    } else if (wantedExtent.width > caps.maxImageExtent.width) {
        wantedExtent.width = caps.maxImageExtent.width;
    }

    if (wantedExtent.height < caps.minImageExtent.height) {
        wantedExtent.height = caps.minImageExtent.height;
    } else if (wantedExtent.height > caps.maxImageExtent.height) {
        wantedExtent.height = caps.maxImageExtent.height;
    }

    uint32_t supported_formats_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, surface, &supported_formats_count,  VK_NULL_HANDLE))

    std::vector<VkSurfaceFormatKHR> supported_formats { supported_formats_count };
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(context.physical_device, surface, &supported_formats_count,  supported_formats.data()))

    uint32_t supported_present_modes_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, surface, &supported_present_modes_count, VK_NULL_HANDLE))

    std::vector<VkPresentModeKHR> supported_present_modes { supported_present_modes_count };
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(context.physical_device, surface, &supported_present_modes_count, supported_present_modes.data()))

    VkSurfaceFormatKHR surface_format = supported_formats[0];
    VkSwapchainKHR old_swapchain_handle = (old_swapchain != std::nullopt) ? old_swapchain.value().handle : VK_NULL_HANDLE;

    VkSwapchainCreateInfoKHR create_info    = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.surface                     = surface;
    create_info.minImageCount               = min_image_count;
    create_info.imageFormat                 = surface_format.format;
    create_info.imageColorSpace             = surface_format.colorSpace;
    create_info.imageExtent                 = wantedExtent;
    create_info.imageArrayLayers            = 1;
    create_info.imageUsage                  = usages;
    create_info.imageSharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform                = caps.currentTransform;
    create_info.compositeAlpha              = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode                 = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped                     = VK_TRUE;
    create_info.oldSwapchain                = VK_NULL_HANDLE;

    VKRESULT(vkCreateSwapchainKHR(context.device, &create_info, nullptr, &swapchain.handle))

    if (old_swapchain != std::nullopt) {
        destroy_swapchain(old_swapchain.value());
    }

    uint32_t image_count = 0;
    VKRESULT(vkGetSwapchainImagesKHR(context.device, swapchain.handle, &image_count, VK_NULL_HANDLE))

    swapchain.image_count = image_count;
    swapchain.images = std::vector<Image>{ swapchain.image_count };
    auto images = std::vector<VkImage>{ swapchain.image_count };
    auto views = std::vector<VkImageView>{ swapchain.image_count };

    VKRESULT(vkGetSwapchainImagesKHR(context.device, swapchain.handle, (uint32_t*)&swapchain.image_count, images.data()))

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel            = 0;
    image_subresource_range.levelCount              = 1;
    image_subresource_range.baseArrayLayer          = 0;
    image_subresource_range.layerCount              = 1;

    for (size_t image_index = 0; image_index < swapchain.image_count; image_index++) {
        swapchain.images[image_index].handle = images[image_index];
        swapchain.images[image_index].subresource_range = image_subresource_range;

        VkImageViewCreateInfo image_view_create_info    = {};
        image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext                    = nullptr;
        image_view_create_info.flags                    = 0;
        image_view_create_info.image                    = swapchain.images[image_index].handle;
        image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                   = surface_format.format;
        image_view_create_info.components               = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_create_info.subresourceRange        = image_subresource_range;

        VKRESULT(vkCreateImageView(context.device, &image_view_create_info, nullptr, &swapchain.images[image_index].view))
    }

    return std::move(swapchain);
}

void vkapi::destroy_swapchain(Swapchain swapchain) {
    for(auto &image: swapchain.images) {
        vkDestroyImageView(context.device, image.view, nullptr);
    }
    vkDestroySwapchainKHR(context.device, swapchain.handle, nullptr);
}


void vkapi::update_descriptor_set_buffer(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding,Buffer& buffer) {
    VkDescriptorBufferInfo descriptor_buffer_info = {};
    descriptor_buffer_info.buffer           = buffer.handle;
    descriptor_buffer_info.offset           = 0;
    descriptor_buffer_info.range            = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor   = {};
    write_descriptor.sType                  = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor.pNext                  = nullptr;
    write_descriptor.dstSet                 = set;
    write_descriptor.dstBinding             = 0;
    write_descriptor.dstArrayElement        = 0;
    write_descriptor.descriptorCount        = 1;
    write_descriptor.descriptorType         = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor.pImageInfo             = VK_NULL_HANDLE;
    write_descriptor.pBufferInfo            = &descriptor_buffer_info;
    write_descriptor.pTexelBufferView       = VK_NULL_HANDLE;

    vkUpdateDescriptorSets(context.device, 1, &write_descriptor, 0, nullptr);
}

void vkapi::update_descriptor_set_image(VkDescriptorSet set, VkDescriptorSetLayoutBinding binding, VkImageView view) {
    VkDescriptorImageInfo descriptor_image_info = {};
    descriptor_image_info.sampler               = VK_NULL_HANDLE;
    descriptor_image_info.imageView             = view;
    descriptor_image_info.imageLayout           = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write_descriptor       = {};
    write_descriptor.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor.pNext                      = nullptr;
    write_descriptor.dstSet                     = set;
    write_descriptor.dstBinding                 = 1;
    write_descriptor.dstArrayElement            = 0;
    write_descriptor.descriptorCount            = binding.descriptorCount;
    write_descriptor.descriptorType             = binding.descriptorType;
    write_descriptor.pImageInfo                 = &descriptor_image_info;
    write_descriptor.pBufferInfo                = VK_NULL_HANDLE;
    write_descriptor.pTexelBufferView           = VK_NULL_HANDLE;

    vkUpdateDescriptorSets(context.device, 1, &write_descriptor, 0, nullptr);
}
