#include "dx12-command-buffer.hpp"

#include "dx12-device.hpp"

namespace dx12 {
    void command_buffer::start() const {
        HRESULT hr = command_allocator->Reset();
        assert(hr == S_OK);

        hr = dx12_command_buffer->Reset(command_allocator, nullptr);
        assert(hr == S_OK);

        auto& device = dx12::device::get_render_device();
        dx12_command_buffer->SetDescriptorHeaps(2U, dx12::device::get_bindless_model().get_heaps());
    }

    void command_buffer::stop() const {
        HRESULT hr = dx12_command_buffer->Close();
        assert(hr == S_OK);
    }

    void command_buffer::copy(handle<device_buffer> src_buffer, handle<device_texture> dst_texture, uint64_t offset) const {
        auto& dst = device::get_render_device().get_texture(dst_texture);
        D3D12_TEXTURE_COPY_LOCATION dst_copy_location {
            .pResource = dst.resource,
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = 0U,
        };

        auto& src = device::get_render_device().get_buffer(src_buffer);
        D3D12_TEXTURE_COPY_LOCATION src_copy_location {
            .pResource = src.resource,
            .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = {
                .Offset = offset,
                .Footprint = {
                    .Format = dst.format,
                    .Width = dst.width,
                    .Height = dst.height,
                    .Depth = dst.depth,
                    .RowPitch = static_cast<UINT>(dst.row_pitch),
                },
            },
        };
        dx12_command_buffer->CopyTextureRegion(&dst_copy_location, 0U, 0U, 0U, &src_copy_location, nullptr);
    }
}
