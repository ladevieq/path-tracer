#pragma once

#include <d3d12.h>
#include <dxgi1_3.h>

#include "freelist.hpp"
#include "dx12-device-types.hpp"

#include "D3D12MemAlloc.h"

namespace dx12 {
    struct command_buffer;

    class device {
        struct queue {
            ID3D12CommandQueue* dx_queue;
            D3D12_COMMAND_LIST_TYPE type;
        };
        public:
        void init();

        static inline device& get_render_device() {
            return render_device;
        }

        handle<device_surface> create_surface(const surface_desc& desc);

        handle<device_buffer> create_buffer(const buffer_desc& desc);

        handle<device_texture> create_texture(const texture_desc& desc);

        void create_graphics_pipeline() {};

        void create_compute_pipeline() {};

        void allocate_command_buffers(command_buffer* buffers, size_t count, QueueType type);


        [[nodiscard]] inline device_surface& get_surface(handle<device_surface> handle) {
            return surfaces[handle];
        }

        [[nodiscard]] inline device_buffer& get_buffer(handle<device_buffer> handle) {
            return buffers[handle];
        }

        [[nodiscard]] inline device_texture& get_texture(handle<device_texture> handle) {
            return textures[handle];
        }

        [[nodiscard]] inline ID3D12Device* get_device() const {
            return device;
        }

        uint32_t submit(command_buffer* buffers, uint32_t count);

        void wait();

        private:
        IDXGIFactory3*              factory;
        IDXGIAdapter*               adapter;
        ID3D12Debug*                debug_interface;

        D3D12MA::Allocator*         allocator;

        ID3D12Device*               device;
        ID3D12Fence*                fence;
        uint64_t                    fence_value = 0U;
        queue                       queues[static_cast<uint32_t>(QueueType::MAX)];

        freelist<device_surface>    surfaces;
        freelist<device_buffer>     buffers;
        freelist<device_texture>    textures;

        static constexpr size_t     max_allocable_command_buffers  = 16U;
        static constexpr size_t     max_submitable_command_buffers = 16U;

        static class device         render_device;

        struct bindless_model {
            void init();

            [[nodiscard]] inline ID3D12DescriptorHeap* const* get_heaps() const {
                return &resources_heap;
            }

            ID3D12DescriptorHeap*   resources_heap;
            ID3D12DescriptorHeap*   samplers_heap;
        } model;

        public:
        [[nodiscard]] static inline const device::bindless_model& get_bindless_model() {
            return render_device.model;
        }
    };

}
