#pragma once

#include "handle.hpp"
#include "freelist.hpp"
#include "texture.hpp"
#include "buffer.hpp"

class resourcemanager {
public:

    handle<texture> create_texture(const texture_desc& desc);

    handle<buffer> create_buffer(const buffer_desc& desc);

    const texture& get_texture(handle<texture> handle);

    const buffer& get_buffer(handle<buffer> handle);

private:
    freelist<texture> textures;
    freelist<buffer> buffers;
};
