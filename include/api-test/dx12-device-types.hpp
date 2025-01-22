#include <cstdint>
#include <dxgi.h>

namespace dx12 {
    struct texture_desc {
    };

    struct device_texture {
    };

    struct surface_desc {
        HWND                    window_handle;
        DXGI_FORMAT             surface_format;
        DXGI_SWAP_EFFECT        present_mode;
        // VkImageUsageFlags         usages;
        DXGI_USAGE              usages;
        uint32_t                image_count         = surface_desc::default_image_count;

        static constexpr uint32_t max_image_count     = 3U;
        static constexpr uint32_t default_image_count = 3U;
    };

    struct device_surface {
        IDXGISwapChain* swapchain;
    };
}
