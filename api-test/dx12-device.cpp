#include "dx12-device.hpp"

#include <cstdint>
#include <cstdio>

dx12device dx12device::render_device = dx12device();

void dx12device::init() {
    SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory)));

    uint32_t index = 0U;
    while (factory->EnumAdapters(index, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC desc;
        SUCCEEDED(adapter->GetDesc(&desc));
        printf("%S\n", desc.Description);
        index++;
    }

    SUCCEEDED(factory->EnumAdapters(0U, &adapter));

    SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)));

    D3D12_COMMAND_QUEUE_DESC queue_desc{
        .Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
        .Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0U,
    };
    SUCCEEDED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[0U])));

    queue_desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
    SUCCEEDED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[1U])));

    queue_desc.Type = D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY;
    queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
    SUCCEEDED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[2U])));
}

void dx12device::create_surface(const surface_desc& desc)
    DXGI_MODE_DESC mode_desc{
        .Width = desc.
    };
    DXGI_SAMPLE_DESC sample_desc{

    };
    DXGI_SWAP_CHAIN_DESC swapchain_desc{
        .BufferDesc = mode_desc,
        .SampleDesc = sample_desc,
        .BufferUsage = DXGI_USAGE_UNORDERED_ACCESS | DXGI_USAGE_RENDER_TARGET_OUTPUT,
    };
    factory->CreateSwapChain(device, )
}
