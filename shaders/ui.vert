struct Vertex {
    vec2    pos;
    vec2    uv;
    uint    col;
};

layout(set = 0, binding = 0) uniform Vertices {
    Vertex[10000] vertices;
};

out gl_PerVertex
{
  vec4 gl_Position;
};

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex].pos, 0.0, 1.0);
}
