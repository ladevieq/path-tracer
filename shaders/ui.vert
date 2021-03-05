struct Vertex {
    vec2    pos;
    vec2    uv;
    uint    color;
};

layout(set = 0, binding = 0) uniform Vertices {
    Vertex vertices[158];
};

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out struct {
    vec2 uv;
    vec4 color;
} Out;

const vec2 scale = vec2(2.0 / 384.0, 2.0 / 186.0);
const vec2 translate = vec2(-1.0, -1.0);

void main() {
    Vertex v = vertices[gl_VertexIndex];

    Out.color = unpackUnorm4x8(v.color);
    Out.uv = v.uv;

    gl_Position = vec4(v.pos * scale + translate, 0.0, 1.0);
}
