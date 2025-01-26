#include <d3d12.h>

#include "handle.hpp"
#include "dx12-device-types.hpp"

namespace dx12 {
    struct command_buffer {
        void start() const;
        void stop() const;

        // void            barrier(handle<device_texture> texture_handle, VkPipelineStageFlags2 stage, VkAccessFlags2 access, VkImageLayout layout) const;

        void copy(handle<device_buffer> src_buffer, handle<device_texture> dst_texture, uint64_t offset = 0) const;

        ID3D12CommandAllocator* command_allocator;
        ID3D12GraphicsCommandList* dx12_command_buffer;
        dx12::QueueType       queue_type;
    };

    struct graphics_command_buffer: public command_buffer {
        // void dispatch(const dispatch_params& params) const;

        // void begin_renderpass(const renderpass_params& params) const;
        // void draw(const draw_params& params) const;
        // void draw_indexed(const draw_indexed_params& params) const;
        // void end_renderpass() const;
    };
}
