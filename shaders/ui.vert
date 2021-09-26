#extension GL_EXT_buffer_reference : require

struct vertex {
    vec2    pos;
    vec2    uv;
    uint    color;
    uint[3] padding;
};

layout(buffer_reference) readonly buffer vertices {
    vertex vertices[];
};

layout(push_constant) uniform constants {
    layout(offset = 64) vertices vertex_buffer;
    vec2 scale;
    vec2 translate;
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
    Out.uv = v.uv;

    gl_Position = vec4(v.pos * consts.scale + consts.translate, 0.0, 1.0);
}
