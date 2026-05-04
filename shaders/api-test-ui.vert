#extension GL_EXT_buffer_reference : require

struct vertex {
    float    pos_x;
    float    pos_y;
    float    uv_x;
    float    uv_y;
    uint    color;
};

struct camera {
    vec4 position;

    vec4 forward;
    vec4 up;
    vec4 right;

    vec4 horizontal;
    vec4 vertical;
    vec4 first_pixel;

    float lens_radius;
};

layout(buffer_reference) readonly buffer global_params {
    camera cam;

    uint max_bounce;
    uint min_bounce;
    uint width;
    uint height;
    uint sample_index;

    uint enable_dof;
    uint debug_bvh;
    int downscale_factor;
};

layout(buffer_reference) readonly buffer pass_params {
    uint padding;
};

layout(buffer_reference) readonly buffer vertices {
    vertex vertices[];
};

layout(buffer_reference) readonly buffer params {
    vertices vertex_buffer;
    vec2 scale;
    vec2 translate;
    uint texture_index;
    uint padding;
};

layout(push_constant) uniform registers {
    global_params globals;
    pass_params pass;
    params ui_params;
} regs;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} Out;


void main() {
    vertex v = regs.ui_params.vertex_buffer.vertices[gl_VertexIndex];

    Out.color = unpackUnorm4x8(v.color);
    Out.uv = vec2(v.uv_x, v.uv_y);

    vec2 pos = vec2(v.pos_x, v.pos_y);
    gl_Position = vec4(pos * regs.ui_params.scale + regs.ui_params.translate, 0.0, 1.0);
}
