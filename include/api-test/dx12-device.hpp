#include <d3d12.h>

#include "freelist.hpp"
#include "dx12-device-types.hpp"

class dx12device {
    public:
    void init();

    static inline dx12device& get_render_device() {
        return render_device;
    }

    handle<dx12::device_surface> create_surface(const dx12::surface_desc& desc);

    void create_buffer() {};

    void create_texture() {};

    void create_graphics_pipeline() {};

    void create_compute_pipeline() {};

    void allocate_command_buffers() {};

    void get_buffer() {};

    void get_texture() {};

    [[nodiscard]] inline dx12::device_surface& get_surface(handle<dx12::device_surface> handle) {
        return surfaces[handle];
    }

    private:
    IDXGIFactory* factory;
    IDXGIAdapter* adapter;

    ID3D12Device* device;
    ID3D12CommandQueue* queues[3U];

    freelist<dx12::device_surface>    surfaces;

    static dx12device render_device;
};
