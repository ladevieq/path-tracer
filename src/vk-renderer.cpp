#include "vk-renderer.hpp"
#include "utils.hpp"

#define VMA_IMPLEMENTATION
#include "thirdparty/vk_mem_alloc.h"

#include <iostream>
#include <vector>
#include <cstring>

#define VKRESULT(result) assert(result == VK_SUCCESS);

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT *data,
                                             void *userData)
{
    std::cerr << data->pMessage << std::endl;
    return VK_FALSE;
}

vkrenderer::vkrenderer(window& wnd) {
    load_vulkan();

    create_instance();
    create_surface(wnd);
    create_device();
    create_swapchain(wnd);
    create_pipeline();
    create_memory_allocator();
    create_command_buffer();
    create_descriptor_set();
    create_fence();
    create_semaphores();
}

void vkrenderer::compute(const input_data& inputs, size_t width, size_t height) {
    VKRESULT(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquire_semaphore, VK_NULL_HANDLE, &current_image_index))

    fill_descriptor_set(inputs);

    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.pNext                    = nullptr;
    cmd_buf_begin_info.flags                    = 0;
    cmd_buf_begin_info.pInheritanceInfo         = nullptr;

    vkBeginCommandBuffer(command_buffer, &cmd_buf_begin_info);

    VkImageSubresourceRange image_subresource_range = {};
    image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    image_subresource_range.baseMipLevel            = 0;
    image_subresource_range.levelCount              = 1;
    image_subresource_range.baseArrayLayer          = 0;
    image_subresource_range.layerCount              = 1;

    VkImageMemoryBarrier image_memory_barrier   = {};
    image_memory_barrier.sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext                  = nullptr;
    image_memory_barrier.srcAccessMask          = 0;
    image_memory_barrier.dstAccessMask          = 0;
    image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_GENERAL;
    image_memory_barrier.srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image                  = swapchain_images[current_image_index];
    image_memory_barrier.subresourceRange       = image_subresource_range;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_memory_barrier
    );

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_layout, 0, 1, &compute_shader_set, 0, nullptr);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline);

    vkCmdDispatch(command_buffer, width / 8, height / 8, 1);

    image_memory_barrier.oldLayout              = VK_IMAGE_LAYOUT_GENERAL;
    image_memory_barrier.newLayout              = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkCmdPipelineBarrier(
        command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_DEPENDENCY_DEVICE_GROUP_BIT,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &image_memory_barrier
    );

    VKRESULT(vkEndCommandBuffer(command_buffer))

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    VkSubmitInfo submit_info            = {};
    submit_info.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                   = nullptr;
    submit_info.waitSemaphoreCount      = 1;
    submit_info.pWaitSemaphores         = &acquire_semaphore;
    submit_info.pWaitDstStageMask       = &wait_stage;
    submit_info.signalSemaphoreCount    = 0;
    submit_info.pSignalSemaphores       = nullptr;
    submit_info.commandBufferCount      = 1;
    submit_info.pCommandBuffers         = &command_buffer;

    VKRESULT(vkQueueSubmit(compute_queue, 1, &submit_info, submission_fence))

    VkPresentInfoKHR present_info   = {};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = 0;
    present_info.pWaitSemaphores    = 0;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &swapchain;
    present_info.pImageIndices      = &current_image_index;
    present_info.pResults           = nullptr;

    VKRESULT(vkQueuePresentKHR(compute_queue, &present_info))

    VKRESULT(vkWaitForFences(device, 1, &submission_fence, VK_TRUE, UINT64_MAX))
}


void vkrenderer::create_instance() {
    VkApplicationInfo app_info   = {};
    app_info.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName    = "path-tracer"; 
    app_info.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName         = "gpu-path-tracer";
    app_info.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion          = VK_API_VERSION_1_2;


    std::vector<const char*> layers = {
        "VK_LAYER_KHRONOS_validation",
    };
    std::vector<const char*> extensions = {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        "VK_KHR_surface",
        "VK_KHR_xcb_surface"
    };

    VkInstanceCreateInfo instance_create_info       = {};
    instance_create_info.sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pApplicationInfo           = &app_info;
    instance_create_info.enabledLayerCount          = layers.size();
    instance_create_info.ppEnabledLayerNames        = layers.data();
    instance_create_info.enabledExtensionCount      = extensions.size();
    instance_create_info.ppEnabledExtensionNames    = extensions.data();

    VKRESULT(vkCreateInstance(&instance_create_info, nullptr, &instance))

    load_instance_functions(instance);

    create_debugger();

    std::cerr << "Instance ready" << std::endl;
}

void vkrenderer::create_debugger() {
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengeCreateInfo {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        nullptr,
        0,
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        DebugCallback,
        nullptr,
    };

    VKRESULT(vkCreateDebugUtilsMessengerEXT(
        instance,
        &debugUtilsMessengeCreateInfo,
        nullptr,
        &debugMessenger
    ))
}

void vkrenderer::create_surface(window& wnd) {
    VkXcbSurfaceCreateInfoKHR create_info   = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.connection                  = wnd.connection;
    create_info.window                      = wnd.win;

    VKRESULT(vkCreateXcbSurfaceKHR(instance, &create_info, nullptr, &platform_surface))
}

void vkrenderer::create_device() {
    select_physical_device();
    select_compute_queue();

    VkDeviceQueueCreateInfo queue_create_info   = {};
    queue_create_info.sType                     = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex          = compute_queue_index;
    queue_create_info.queueCount                = 1;

    float queue_priority                        = 1.f;
    queue_create_info.pQueuePriorities          = &queue_priority;

    VkPhysicalDeviceFeatures device_features    = {};

    std::vector<const char*> device_ext         = {
        "VK_KHR_swapchain"
    };

    VkDeviceCreateInfo device_create_info       = {};
    device_create_info.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos        = &queue_create_info;
    device_create_info.queueCreateInfoCount     = 1;
    device_create_info.pEnabledFeatures         = &device_features;
    device_create_info.ppEnabledExtensionNames  = device_ext.data();
    device_create_info.enabledExtensionCount    = device_ext.size();
    device_create_info.enabledLayerCount        = 0; // Deprecated https://www.khronos.org/registry/vulkan/specs/1.2/html/chap31.html#extendingvulkan-layers-devicelayerdeprecation

    VKRESULT(vkCreateDevice(physical_device, &device_create_info, nullptr, &device))

    load_device_functions(device);

    vkGetDeviceQueue(device, compute_queue_index, 0, &compute_queue);

    std::cerr << "Device ready" << std::endl;
}

void vkrenderer::create_swapchain(window& wnd) {
    VkBool32 queue_support_presentation = VK_FALSE;
    VKRESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, compute_queue_index, platform_surface, &queue_support_presentation));

    VkSurfaceCapabilitiesKHR caps;
    VKRESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, platform_surface, &caps))

    uint32_t supported_formats_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, platform_surface, &supported_formats_count,  VK_NULL_HANDLE))

    std::vector<VkSurfaceFormatKHR> supported_formats { supported_formats_count };
    VKRESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, platform_surface, &supported_formats_count,  supported_formats.data()))

    uint32_t supported_present_modes_count = 0;
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, platform_surface, &supported_present_modes_count, VK_NULL_HANDLE))

    std::vector<VkPresentModeKHR> supported_present_modes { supported_present_modes_count };
    VKRESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, platform_surface, &supported_present_modes_count, supported_present_modes.data()))

    surface_format = supported_formats[0];

    VkSwapchainCreateInfoKHR create_info    = {};
    create_info.sType                       = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.pNext                       = nullptr;
    create_info.flags                       = 0;
    create_info.surface                     = platform_surface;
    create_info.minImageCount               = swapchain_images_count;
    create_info.imageFormat                 = surface_format.format;
    create_info.imageColorSpace             = surface_format.colorSpace;
    create_info.imageExtent                 = caps.currentExtent;
    create_info.imageArrayLayers            = 1;
    create_info.imageUsage                  = VK_IMAGE_USAGE_STORAGE_BIT;
    create_info.imageSharingMode            = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform                = caps.currentTransform;
    create_info.compositeAlpha              = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode                 = VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped                     = VK_TRUE;
    create_info.oldSwapchain                = VK_NULL_HANDLE;

    VKRESULT(vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain))

    swapchain_images_count = 0;
    VKRESULT(vkGetSwapchainImagesKHR(device, swapchain, (uint32_t*)&swapchain_images_count, VK_NULL_HANDLE))

    swapchain_images = std::vector<VkImage>{ swapchain_images_count };
    swapchain_images_views = std::vector<VkImageView>{ swapchain_images_count };
    VKRESULT(vkGetSwapchainImagesKHR(device, swapchain, (uint32_t*)&swapchain_images_count, swapchain_images.data()))

    for (size_t index = 0; index < swapchain_images_count; index++) {
        VkImageSubresourceRange image_subresource_range = {};
        image_subresource_range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
        image_subresource_range.baseMipLevel            = 0;
        image_subresource_range.levelCount              = 1;
        image_subresource_range.baseArrayLayer          = 0;
        image_subresource_range.layerCount              = 1;

        VkImageViewCreateInfo image_view_create_info    = {};
        image_view_create_info.sType                    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext                    = nullptr;
        image_view_create_info.flags                    = 0;
        image_view_create_info.image                    = swapchain_images[index];
        image_view_create_info.viewType                 = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.format                   = surface_format.format;
        image_view_create_info.components               = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        image_view_create_info.subresourceRange        = image_subresource_range;

        VKRESULT(vkCreateImageView(device, &image_view_create_info, nullptr, &swapchain_images_views[index]))
    }
}

void vkrenderer::create_pipeline() {
    std::vector<uint8_t> shader_code = get_shader_code("./shaders/compute.comp.spv");
    if (shader_code.size() == 0) {
        exit(1);
    }

    VkShaderModule compute_shader_module;
    VkShaderModuleCreateInfo shader_create_info     = {};
    shader_create_info.sType                        = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext                        = nullptr;
    shader_create_info.flags                        = 0;
    shader_create_info.codeSize                     = shader_code.size();
    shader_create_info.pCode                        = (uint32_t*)shader_code.data();

    VKRESULT(vkCreateShaderModule(device, &shader_create_info, nullptr, &compute_shader_module))

    VkPipelineShaderStageCreateInfo stage_create_info = {};
    stage_create_info.sType                         = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage_create_info.pNext                         = nullptr;
    stage_create_info.flags                         = 0;
    stage_create_info.stage                         = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_create_info.module                        = compute_shader_module;
    stage_create_info.pName                         = "main";
    stage_create_info.pSpecializationInfo           = nullptr;



    std::vector<VkDescriptorSetLayoutBinding> bindings {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = VK_NULL_HANDLE,
        }
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.pNext         = nullptr;
    descriptor_set_layout_create_info.flags         = 0;
    descriptor_set_layout_create_info.bindingCount  = bindings.size();
    descriptor_set_layout_create_info.pBindings     = bindings.data();

    VKRESULT(vkCreateDescriptorSetLayout(device, &descriptor_set_layout_create_info, nullptr, &compute_shader_layout))

    VkPipelineLayoutCreateInfo layout_create_info   = {};
    layout_create_info.sType                        = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.pNext                        = nullptr;
    layout_create_info.flags                        = 0;
    layout_create_info.setLayoutCount               = 1;
    layout_create_info.pSetLayouts                  = &compute_shader_layout;
    layout_create_info.pushConstantRangeCount       = 0;
    layout_create_info.pPushConstantRanges          = nullptr;

    VKRESULT(vkCreatePipelineLayout(device, &layout_create_info, nullptr, &compute_pipeline_layout))

    VkComputePipelineCreateInfo create_info         = {};
    create_info.sType                               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    create_info.pNext                               = nullptr;
    create_info.flags                               = 0;
    create_info.stage                               = stage_create_info;
    create_info.layout                              = compute_pipeline_layout;

    VKRESULT(vkCreateComputePipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &create_info,
        nullptr,
        &compute_pipeline
    ))

    std::cerr << "Compute pipeline ready" << std::endl;
}

void vkrenderer::create_memory_allocator() {
    VmaAllocatorCreateInfo allocator_info = {};
    allocator_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_info.physicalDevice = physical_device;
    allocator_info.device = device;
    allocator_info.instance = instance;
     
    vmaCreateAllocator(&allocator_info, &allocator);
}

void vkrenderer::create_command_buffer() {
    VkCommandPoolCreateInfo cmd_pool_create_info    = {};
    cmd_pool_create_info.sType                      = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext                      = nullptr;
    cmd_pool_create_info.flags                      = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmd_pool_create_info.queueFamilyIndex           = compute_queue_index;

    VKRESULT(vkCreateCommandPool(device, &cmd_pool_create_info, nullptr, &command_pool))

    VkCommandBufferAllocateInfo cmd_buf_allocate_info   = {};
    cmd_buf_allocate_info.sType                         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_allocate_info.pNext                         = nullptr;
    cmd_buf_allocate_info.commandPool                   = command_pool;
    cmd_buf_allocate_info.level                         = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_allocate_info.commandBufferCount            = 1;

    vkAllocateCommandBuffers(device, &cmd_buf_allocate_info, &command_buffer);
}
void vkrenderer::create_descriptor_set() {
    std::vector<VkDescriptorPoolSize> descriptor_pools_sizes = {
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount                    = 1,
        },
        {
            .type                               = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount                    = 1,
        }
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info  = {};
    descriptor_pool_create_info.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext                       = nullptr;
    descriptor_pool_create_info.flags                       = 0;
    descriptor_pool_create_info.maxSets                     = 1;
    descriptor_pool_create_info.poolSizeCount               = descriptor_pools_sizes.size();
    descriptor_pool_create_info.pPoolSizes                  = descriptor_pools_sizes.data();

    VKRESULT(vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptor_pool))

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info    = {};
    descriptor_set_allocate_info.sType                          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext                          = nullptr;
    descriptor_set_allocate_info.descriptorPool                 = descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount             = 1;
    descriptor_set_allocate_info.pSetLayouts                    = &compute_shader_layout;

    vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &compute_shader_set);
}

void vkrenderer::create_fence() {
    VkFenceCreateInfo create_info   = {};
    create_info.sType               = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.pNext               = nullptr;
    create_info.flags               = 0;

    VKRESULT(vkCreateFence(device, &create_info, nullptr, &submission_fence))
}

void vkrenderer::create_semaphores() {
    VkSemaphoreCreateInfo create_info   = {};
    create_info.sType                   = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext                   = nullptr;
    create_info.flags                   = 0;

    VKRESULT(vkCreateSemaphore(device, &create_info, nullptr, &execution_semaphore))
    VKRESULT(vkCreateSemaphore(device, &create_info, nullptr, &acquire_semaphore))
}

void vkrenderer::select_physical_device() {
    uint32_t physical_devices_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);

    if (physical_devices_count < 1) {
        std::cerr << "No physical device found !" << std::endl;
        exit(1);
    }

    std::vector<VkPhysicalDevice> physical_devices { physical_devices_count };
    vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data());

    physical_device = physical_devices[0];
}

void vkrenderer::select_compute_queue() {
    uint32_t queue_properties_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties { queue_properties_count };
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_properties_count, queue_properties.data());

    for (uint32_t properties_index = 0; properties_index < queue_properties_count; properties_index++) {
        if (queue_properties[properties_index].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            compute_queue_index = properties_index;
            return;
        }
    }

    std::cerr << "No compute queue found !" << std::endl;
    exit(1);
}

void vkrenderer::fill_descriptor_set(const input_data& inputs) {
    // Update uniform buffer binding
    VkBufferCreateInfo buffer_info  = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    buffer_info.size                = sizeof(inputs);
    buffer_info.usage               = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
     
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
     
    VmaAllocation allocation;
    vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &compute_shader_buffer, &allocation, nullptr);

    vmaMapMemory(allocator, allocation, (void**) &mapped_data);
    std::memcpy(&mapped_data, &inputs, sizeof(inputs));

    VkDescriptorBufferInfo descriptor_buf_info      = {};
    descriptor_buf_info.buffer                      = compute_shader_buffer;
    descriptor_buf_info.offset                      = 0;
    descriptor_buf_info.range                       = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write_descriptor_set       = {};
    write_descriptor_set.sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set.pNext                      = nullptr;
    write_descriptor_set.dstSet                     = compute_shader_set;
    write_descriptor_set.dstBinding                 = 0;
    write_descriptor_set.dstArrayElement            = 0;
    write_descriptor_set.descriptorCount            = 1;
    write_descriptor_set.descriptorType             = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write_descriptor_set.pImageInfo                 = VK_NULL_HANDLE;
    write_descriptor_set.pBufferInfo                = &descriptor_buf_info;
    write_descriptor_set.pTexelBufferView           = VK_NULL_HANDLE;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);


    // Update storage image binding
    VkDescriptorImageInfo descriptor_image_info     = {};
    descriptor_image_info.sampler                   = VK_NULL_HANDLE;
    descriptor_image_info.imageView                 = swapchain_images_views[current_image_index];
    descriptor_image_info.imageLayout               = VK_IMAGE_LAYOUT_GENERAL;

    write_descriptor_set.dstBinding                 = 1;
    write_descriptor_set.descriptorType             = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write_descriptor_set.pImageInfo                 = &descriptor_image_info;
    write_descriptor_set.pBufferInfo                = VK_NULL_HANDLE;

    vkUpdateDescriptorSets(device, 1, &write_descriptor_set, 0, nullptr);
}
