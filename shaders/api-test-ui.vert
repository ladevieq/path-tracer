#extension GL_EXT_buffer_reference : require

struct vertex {
    float    pos_x;
    float    pos_y;
    float    uv_x;
    float    uv_y;
    uint    color;
};

layout(buffer_reference, buffer_reference_align = 8) readonly buffer params {
    vec2 scale;
    vec2 translate;
    uint texture_index;
    uint padding;
};

layout(buffer_reference) readonly buffer vertices {
    vertex vertices[];
};

layout(push_constant) uniform constants {
    layout(offset = 16) params ui_params;
    vertices vertex_buffer;
} consts;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} Out;


void main() {
    vertex v = consts.vertex_buffer.vertices[gl_VertexIndex];

    Out.color = unpackUnorm4x8(v.color);
    Out.uv = vec2(v.uv_x, v.uv_y);

    vec2 pos = vec2(v.pos_x, v.pos_y);
    gl_Position = vec4(pos * consts.ui_params.scale + consts.ui_params.translate, 0.0, 1.0);
}
