#include <d3d12.h>
#include <dxgi.h>

class dx12device {
    public:
    void init();

    static inline dx12device& get_render_device() {
        return render_device;
    }

    void create_surface(const surface_desc& desc);

    void create_buffer() {};

    void create_texture() {};

    void create_graphics_pipeline() {};

    void create_compute_pipeline() {};

    void allocate_command_buffers() {};

    void get_buffer() {};

    void get_texture() {};

    private:
    IDXGIFactory* factory;
    IDXGIAdapter* adapter;

    ID3D12Device* device;
    ID3D12CommandQueue queues[3U];

    static dx12device render_device;
};
