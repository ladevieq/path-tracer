const float PI = 3.141592653589793f;
const float TWO_PI = PI * 2.0f;

vec3 less_than(vec3 vec, float value)
{
    return vec3(
        (vec.x < value) ? 1.0f : 0.0f,
        (vec.y < value) ? 1.0f : 0.0f,
        (vec.z < value) ? 1.0f : 0.0f);
}

vec3 sample_hemisphere(vec2 randoms) {
    float radius  = sqrt(randoms[0]);
    float theta = 2.0 * PI * randoms[1];

    return vec3(cos(theta) * radius, sin(theta) * radius, sqrt(1 - randoms[0]));
}

vec2 disk_vec(vec2 randoms) {
    float z  = randoms[0] * 2.0 - 1.0;
    float a = 2.0 * PI * randoms[1];
    float r = sqrt(1.0 - z * z);

    return vec2(cos(a) * r, sin(a) * r);
}

// Calculates rotation quaternion from axis vector to the vector (0, 0, 1)
// Input vector must be normalized!
vec4 rotation_to_z_axis(vec3 axis) {
    // Handle special case when axis is exact or near opposite of (0, 0, 1)
    if (axis.z < -0.99999f) return vec4(1.0f, 0.0f, 0.0f, 0.0f);

    return normalize(vec4(axis.y, -axis.x, 0.0f, 1.0f + axis.z));
}

vec4 invert_rotation(vec4 q) {
    return vec4(-q.xyz, q.w);
}

// Optimized point rotation using quaternion
// Source: https://gamedev.stackexchange.com/questions/28395/rotating-vector3-by-a-quaternion
vec3 rotate_point(vec4 q, vec3 v) {
    const vec3 q_axis = q.xyz;
    return 2.0f * dot(q_axis, v) * q_axis + (q.w * q.w - dot(q_axis, q_axis)) * v + 2.0f * q.w * cross(q_axis, v);
}
