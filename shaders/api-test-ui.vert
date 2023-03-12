#extension GL_EXT_buffer_reference : require

struct vertex {
    float    pos_x;
    float    pos_y;
    float    uv_x;
    float    uv_y;
    uint    color;
};

layout(buffer_reference) readonly buffer vertices {
    vertex vertices[];
};

layout(set = 2, binding = 0) uniform parameters {
    vertices vertex_buffer;
    vec2 scale;
    vec2 translate;
    uint texture_index;
} params;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} Out;


void main() {
    vertex v = params.vertex_buffer.vertices[gl_VertexIndex];

    Out.color = unpackUnorm4x8(v.color);
    Out.uv = vec2(v.uv_x, v.uv_y);

    vec2 pos = vec2(v.pos_x, v.pos_y);
    gl_Position = vec4(pos * params.scale + params.translate, 0.0, 1.0);
}
