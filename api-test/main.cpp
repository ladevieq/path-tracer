#include <cassert>
#include <cstdio>
#include <vk_mem_alloc.h>

#include "window.hpp"
#include "vk-device.hpp"
// #include "resource-manager.hpp"

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

    window wnd { 1280U, 720U };

#ifdef ENABLE_RENDERDOC
    if(rdoc_api != nullptr) rdoc_api->StartFrameCapture(nullptr, nullptr);
#endif

    vkdevice device { wnd };

    auto gpu_texture_handle = device.create_texture({
        .width = 1024U,
        .height = 1024U,
        .usages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .type = VK_IMAGE_TYPE_2D,
    });

    auto gpu_buffer = device.get_buffer(device.create_buffer({
        .size = 1024U * sizeof(uint32_t),
        .usages = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .memory_usage = VMA_MEMORY_USAGE_GPU_ONLY,
    }));

    auto staging_buffer_handle = device.create_buffer({
        .size = 1024U * sizeof(uint32_t),
        .usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .memory_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
    });
    auto staging_buffer = device.get_buffer(staging_buffer_handle);

    uint64_t t = 7;
    (*(uint64_t*)staging_buffer.mapped_ptr) = t;

    transfer_command_buffer command_buffer;
    device.allocate_command_buffers(&command_buffer, 1, QueueType::GRAPHICS);

    command_buffer.start();

    command_buffer.copy(staging_buffer_handle, gpu_texture_handle);

    command_buffer.stop();

    device.submit(&command_buffer, 1);

#ifdef ENABLE_RENDERDOC
    if(rdoc_api != nullptr) rdoc_api->EndFrameCapture(nullptr, nullptr);
#endif

    while(wnd.isOpen) {
        wnd.poll_events();
    }

    return 0;
}
