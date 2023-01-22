#include <cassert>
#include <cstdio>
#include <span>
#include <vk_mem_alloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "window.hpp"
#include "vk-device.hpp"
#include "utils.hpp"

#define ENABLE_RENDERDOC

#ifdef ENABLE_RENDERDOC
#include <renderdoc.h>

RENDERDOC_API_1_4_1 *rdoc_api = nullptr;
#endif

int main() {

#ifdef ENABLE_RENDERDOC
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll")) {
      auto RENDERDOC_GetAPI =
          (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      int ret =
          RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void **)&rdoc_api);
      assert(ret == 1);
    }
#endif

    window wnd { 640U, 360U };

#ifdef ENABLE_RENDERDOC
    if(rdoc_api != nullptr) rdoc_api->StartFrameCapture(nullptr, nullptr);
#endif

    auto* device = vkdevice::init_device(wnd);

    constexpr size_t image_size = 1024U;
    constexpr size_t image_count = 4U;
    auto staging_buffer_handle = device->create_buffer({
        .size = image_size * image_size * sizeof(uint32_t) * image_count,
        .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    });
    const auto& staging_buffer = device->get_buffer(staging_buffer_handle);

    int width;
    int height;
    int channels;
    uint8_t* data = stbi_load("../models/sponza/466164707995436622.jpg", &width, &height, &channels, 4);
    size_t size = static_cast<size_t>(width) * height * 4U;
    memcpy(staging_buffer.mapped_ptr, data, size);

    auto gpu_texture_handle = device->create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    off_t offset = width * height * 4;
    data = stbi_load("../models/sponza/715093869573992647.jpg", &width, &height, &channels, 4);
    size = static_cast<size_t>(width) * height * 4U;
    auto* addr = static_cast<void*>(static_cast<uint8_t*>(staging_buffer.mapped_ptr) + offset);
    memcpy(addr, data, size);

    auto second_texture = device->create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    auto output_texture_handle = device->create_texture({
        .width  = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .usages = VK_IMAGE_USAGE_STORAGE_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type   = VK_IMAGE_TYPE_2D,
    });

    // auto gpu_buffer = vkdevice::get_buffer(device.create_buffer({
    //     .size = 1024U * sizeof(uint32_t),
    //     .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    //     .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    //     .memory_usage = VMA_MEMORY_USAGE_GPU_ONLY,
    // }));

    auto code = read_file("shaders/test.comp.spv");
    auto compute = device->create_pipeline({
        .cs_code = std::span(code),
    });


    graphics_command_buffer command_buffer;
    device->allocate_command_buffers(&command_buffer, 1, QueueType::GRAPHICS);

    command_buffer.start();

    command_buffer.barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_buffer.barrier(second_texture, VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_buffer.copy(staging_buffer_handle, gpu_texture_handle);
    command_buffer.copy(staging_buffer_handle, second_texture, static_cast<VkDeviceSize>(offset));

    command_buffer.barrier(gpu_texture_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);
    command_buffer.barrier(second_texture, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL);
    command_buffer.barrier(output_texture_handle, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL);

    command_buffer.dispatch({
        .pipeline = compute,
        .group_size = { image_size, image_size, 1U },
        .local_group_size = { 8U, 8U, 1U },
    });

    command_buffer.stop();

    device->submit(&command_buffer, 1);

#ifdef ENABLE_RENDERDOC
    if(rdoc_api != nullptr) rdoc_api->EndFrameCapture(nullptr, nullptr);
#endif

    while(wnd.isOpen) {
        wnd.poll_events();
    }

    return 0;
}
