#include "dx12-device.hpp"

#include <cstdint>
#include <cstdio>

#include "dx12-command-buffer.hpp"

namespace dx12 {
    device device::render_device = dx12::device();

    void device::init() {
        HRESULT hr = S_OK;
        hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory));
        assert(hr == S_OK);

        uint32_t index = 0U;
        while (factory->EnumAdapters(index, &adapter) != DXGI_ERROR_NOT_FOUND) {
            DXGI_ADAPTER_DESC desc;
            SUCCEEDED(adapter->GetDesc(&desc));
            printf("%S\n", desc.Description);
            index++;
        }

        SUCCEEDED(factory->EnumAdapters(0U, &adapter));

        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)))) {
            debug_interface->EnableDebugLayer();
        }

        hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device));
        assert(hr == S_OK);

        queues[static_cast<uint32_t>(QueueType::GRAPHICS)].type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        D3D12_COMMAND_QUEUE_DESC queue_desc{
            .Type = queues[static_cast<uint32_t>(QueueType::GRAPHICS)].type,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
            .Flags = D3D12_COMMAND_QUEUE_FLAGS::D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0U,
        };
        device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[static_cast<uint32_t>(QueueType::GRAPHICS)].dx_queue));
        assert(hr == S_OK);

        queue_desc.Type = queues[static_cast<uint32_t>(QueueType::ASYNC_COMPUTE)].type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
        device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[static_cast<uint32_t>(QueueType::ASYNC_COMPUTE)].dx_queue));
        assert(hr == S_OK);

        queue_desc.Type = queues[static_cast<uint32_t>(QueueType::TRANSFER)].type = D3D12_COMMAND_LIST_TYPE_COPY;
        queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH,
        device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queues[static_cast<uint32_t>(QueueType::TRANSFER)].dx_queue));
        assert(hr == S_OK);

        hr = device->CreateFence(0U, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        assert(hr == S_OK);

        D3D12MA::ALLOCATOR_DESC allocator_desc {
            .pDevice = device,
            .pAdapter = adapter,
        };

        hr = D3D12MA::CreateAllocator(&allocator_desc, &allocator);
        assert(hr == S_OK);

        model.init();
    }

    handle<device_surface> device::create_surface(const surface_desc& desc) {
        device_surface surface {
        };
        DXGI_SAMPLE_DESC sample_desc{
            .Count = 1U,
            .Quality = 0U,
        };
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc1{
            .Width = 0U,
            .Height = 0U,
            .Format = desc.surface_format,
            .Stereo = FALSE,
            .SampleDesc = sample_desc,
            .BufferUsage = desc.usages,
            .BufferCount = desc.image_count,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = desc.present_mode,
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = 0U,
        };
        HRESULT hr = factory->CreateSwapChainForHwnd(queues[static_cast<uint32_t>(QueueType::GRAPHICS)].dx_queue, desc.window_handle, &swapchain_desc1, nullptr, nullptr, &surface.swapchain);
        assert(hr == S_OK);

         return surfaces.add(surface);
    }

    handle<device_buffer> device::create_buffer(const buffer_desc& desc) {
        device_buffer buffer {
            .size = desc.size,
        };

        DXGI_SAMPLE_DESC sample_desc {
            .Count = 1U,
            .Quality = 0U,
        };
        D3D12_RESOURCE_DESC resource_desc {
            .Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER,
            .Alignment          = 0U,
            .Width              = buffer.size,
            .Height             = 1U,
            .DepthOrArraySize   = 1U,
            .MipLevels          = 1U,
            .Format             = DXGI_FORMAT_UNKNOWN,
            .SampleDesc         = sample_desc,
            .Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
            .Flags              = desc.usages,
        };
        D3D12MA::ALLOCATION_DESC allocation_desc {
            .HeapType = desc.heap_type,
        };
        HRESULT hr = allocator->CreateResource(&allocation_desc, &resource_desc, desc.states, nullptr, &buffer.alloc, IID_PPV_ARGS(&buffer.resource));
        assert(hr == S_OK);

        if (desc.map) {
            hr = buffer.resource->Map(0U, nullptr, &buffer.mapped_ptr);
            assert(hr == S_OK);
        }

        return buffers.add(buffer);
    }

    handle<device_texture> device::create_texture(const texture_desc& desc) {
        device_texture texture {
            .width = desc.width,
            .height = desc.height,
            .depth = desc.depth,
            .mips = desc.mips,
            .format = desc.format,
        };

        DXGI_SAMPLE_DESC sample_desc {
            .Count = 1U,
            .Quality = 0U,
        };
        D3D12_RESOURCE_DESC resource_desc {
            .Dimension          = desc.type,
            .Alignment          = 0U,
            .Width              = texture.width,
            .Height             = texture.height,
            .DepthOrArraySize   = static_cast<uint16_t>(texture.depth),
            .MipLevels          = static_cast<uint16_t>(texture.mips),
            .Format             = desc.format,
            .SampleDesc         = sample_desc,
            .Layout             = D3D12_TEXTURE_LAYOUT_UNKNOWN,
            .Flags              = desc.usages,
        };
        D3D12MA::ALLOCATION_DESC allocation_desc {
            .HeapType = D3D12_HEAP_TYPE_DEFAULT,
        };

        HRESULT hr = allocator->CreateResource(&allocation_desc, &resource_desc, desc.states, nullptr, &texture.alloc, IID_PPV_ARGS(&texture.resource));
        assert(hr == S_OK);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT place_footprint;
        UINT num_rows = 0U;
        UINT64 total_bytes = 0U;
        device->GetCopyableFootprints(&resource_desc, 0U, 1U, 0U, &place_footprint, &num_rows, &texture.row_pitch, &total_bytes);

        return textures.add(texture);
    }

    void device::allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type) {
        HRESULT hr = S_OK;
        for (uint32_t index = 0U; index < count; index++) {
            buffers[index].queue_type = type;

            auto list_type = queues[static_cast<uint32_t>(type)].type;
            hr = device->CreateCommandAllocator(list_type, IID_PPV_ARGS(&buffers[index].command_allocator));
            assert(hr == S_OK);

            hr = device->CreateCommandList(0U, list_type, buffers[index].command_allocator, nullptr, IID_PPV_ARGS(&buffers[index].dx12_command_buffer));
            assert(hr == S_OK);

            hr = buffers[index].dx12_command_buffer->Close();
            assert(hr == S_OK);
        }
    }

    uint32_t device::submit(command_buffer* buffers, uint32_t count) {
        assert((max_submitable_command_buffers - count) >= 0);
        const auto queue_type = buffers[0].queue_type;
        auto& queue = queues[static_cast<uint32_t>(buffers[0].queue_type)];
        ID3D12CommandList* lists[max_submitable_command_buffers] = { nullptr };
        
        for (auto index{ 0U }; index < count; index++) {
            assert(queue_type == buffers[index].queue_type);

            lists[index] = buffers[index].dx12_command_buffer;
        }

        queue.dx_queue->ExecuteCommandLists(count, lists);
        HRESULT hr = queue.dx_queue->Signal(fence, ++fence_value);
        assert(hr == S_OK);

        for (uint32_t index = 0U; index < count; index++) {
            auto& buffer = buffers[index];

            hr = buffer.dx12_command_buffer->Reset(buffer.command_allocator, nullptr);
            assert(hr == S_OK);
        }

        return fence_value;
    }

    void device::wait() {
        for (auto& queue : queues) {
            queue.dx_queue->Wait(fence, fence_value);
        }
    }

    void device::bindless_model::init() {
        ID3D12Device* device = device::get_render_device().get_device();

        D3D12_DESCRIPTOR_HEAP_DESC resources_descriptor_heap_desc {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            .NumDescriptors = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask       = 0U,
        };
        
        HRESULT hr = device->CreateDescriptorHeap(&resources_descriptor_heap_desc, IID_PPV_ARGS(&resources_heap));
        assert(hr == S_OK);

        D3D12_DESCRIPTOR_HEAP_DESC samplers_descriptor_heap_desc {
            .Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
            .NumDescriptors = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE,
            .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            .NodeMask       = 0U,
        };
        hr = device->CreateDescriptorHeap(&samplers_descriptor_heap_desc, IID_PPV_ARGS(&samplers_heap));
        assert(hr == S_OK);
    }
}
