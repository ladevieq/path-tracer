#pragma once
#include <d3d12.h>
#include <dxgi1_3.h>
#include <D3D12MemAlloc.h>

#include <cstdint>

namespace dx12 {
    enum class QueueType : uint32_t {
        GRAPHICS,
        ASYNC_COMPUTE,
        TRANSFER,
        MAX,
    };

    struct buffer_desc {
        uint32_t                    size;
        bool                        map;

        D3D12_RESOURCE_STATES       states;
        D3D12_RESOURCE_FLAGS        usages;
        D3D12_HEAP_TYPE             heap_type = D3D12_HEAP_TYPE_DEFAULT;
    };

    struct device_buffer {
        ID3D12Resource*         resource;

        D3D12MA::Allocation*    alloc;
        uint32_t                size;
        void*                   mapped_ptr = nullptr;
    };


    struct texture_desc {
        uint32_t                    width  = 1U;
        uint32_t                    height = 1U;
        uint32_t                    depth  = 1U;
        uint32_t                    mips   = 1U;

        D3D12_RESOURCE_STATES       states;
        D3D12_RESOURCE_FLAGS        usages;
        DXGI_FORMAT                 format;
        D3D12_RESOURCE_DIMENSION    type;
    };

    struct device_texture {
        ID3D12Resource*         resource;
        D3D12MA::Allocation*    alloc;

        uint32_t                width       = 1U;
        uint32_t                height      = 1U;
        uint32_t                depth       = 1U;
        uint32_t                mips        = 1U;
        uint64_t                row_pitch   = 1U;

        DXGI_FORMAT             format;

    };

    struct surface_desc {
        HWND                    window_handle;
        DXGI_FORMAT             surface_format;
        DXGI_SWAP_EFFECT        present_mode;
        DXGI_USAGE              usages;
        uint32_t                image_count         = surface_desc::default_image_count;

        static constexpr uint32_t max_image_count     = 3U;
        static constexpr uint32_t default_image_count = 3U;
    };

    struct device_surface {
        IDXGISwapChain1* swapchain;
    };
}
