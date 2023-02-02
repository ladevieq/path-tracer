#extension GL_EXT_buffer_reference : require

struct vertex {
    vec2    pos;
    vec2    uv;
    uint    color;
    uint    padding;
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
    Out.uv = v.uv;

    gl_Position = vec4(v.pos * params.scale + params.translate, 0.0, 1.0);

    // vec2 pos[3] = vec2[3]( vec2(-0.7, 0.7), vec2(0.7, 0.7), vec2(0.0, -0.7) );
    // gl_Position = vec4(pos[gl_VertexIndex], 1.0, 1.0);
}
