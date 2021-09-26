struct ray {
    vec3 origin;
    vec3 direction;
    float min_t;
    float max_t;
};

vec3 at(ray r, float t) {
    return r.origin + r.direction * t;
}

struct hit_info {
    vec3 point;
    vec3 barycentrics;
    vec3 geometry_normal;
    vec3 shading_normal;
    vec3 color;
    vec2 metalness_roughness;
    vec2 uv;
    float t;
    uint id;
};

bool hit_aabb(vec3 minimum, vec3 maximum, ray r) {
    const vec3 invdir = 1.0 / r.direction;
    const vec3 f = (maximum.xyz - r.origin.xyz) * invdir;
    const vec3 n = (minimum.xyz - r.origin.xyz) * invdir;

    const vec3 tmax = max(f, n);
    const vec3 tmin = min(f, n);

    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), r.max_t);
    const float t0 = max(max(max(tmin.x, max(tmin.y, tmin.z)), r.min_t), 0.0f);

    return t1 >= t0;
}

uint hit_aabbs(ray r) {
    uint hit_count = 0;
    int id = 0;

    while(id != -1) {
        if (bufs.bvh.nodes[id].primitive_id != -1) {
            // triangle tri = bufs.geometry.triangles[bufs.bvh.nodes[id].primitive_id];
            // vec3 minimum = min(tri.v1.position, min(tri.v2.position, tri.v3.position)).xyz;
            // vec3 maximum = max(tri.v1.position, max(tri.v2.position, tri.v3.position)).xyz;

            // if (hit_aabb(minimum, maximum, r)) {
            //     hit_count++;
            // }
            id = bufs.bvh.nodes[id].next_id;
        } else if (hit_aabb(bufs.bvh.nodes[id].min, bufs.bvh.nodes[id].max, r)) {
                hit_count++;
                id++;
        } else {
            id = bufs.bvh.nodes[id].next_id;
        }
    }

    return hit_count;
}

bool hit_sphere(sphere s, ray r, out hit_info info) {
    vec3 origin_center = r.origin - s.position.xyz;

    float a = dot(r.direction, r.direction);
    float half_b = dot(r.direction, origin_center);
    float c = dot(origin_center, origin_center) - s.radius * s.radius;
    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0) {
        return false;
    }

    float sqrt_discriminant = sqrt(discriminant);
    float root = (-half_b - sqrt_discriminant) / a;
    if (root < r.min_t || root > r.max_t) {
        root = (-half_b + sqrt_discriminant) / a;

        if (root < r.min_t || root > r.max_t) {
            return false;
        }
    }

    info.t = root;
    // info.point = at(r, info.t);

    // vec3 outward_normal = (info.point - s.position.xyz) / s.radius;
    // bool front_face = dot(outward_normal, r.direction) < 0.0;
    // info.normal = front_face ? outward_normal : -outward_normal;

    return true;
}

bool hit_triangle(uint id, ray r, out hit_info info) {
    uint id1 = bufs.indices_arr.indices[id * 3]        & 0x00ffffff;
    uint id2 = bufs.indices_arr.indices[id * 3 + 1]    & 0x00ffffff;
    uint id3 = bufs.indices_arr.indices[id * 3 + 2]    & 0x00ffffff;

    vec3 p1 = vec3(bufs.positions_arr.positions[id1 * 3], bufs.positions_arr.positions[id1 * 3 + 1], bufs.positions_arr.positions[id1 * 3 + 2]);
    vec3 p2 = vec3(bufs.positions_arr.positions[id2 * 3], bufs.positions_arr.positions[id2 * 3 + 1], bufs.positions_arr.positions[id2 * 3 + 2]);
    vec3 p3 = vec3(bufs.positions_arr.positions[id3 * 3], bufs.positions_arr.positions[id3 * 3 + 1], bufs.positions_arr.positions[id3 * 3 + 2]);

    vec3 v2v1 = p2 - p1;
    vec3 v3v1 = p3 - p1;

    vec3 originv1 = r.origin - p1;
    vec3 normal = cross(v2v1, v3v1);
    vec3 q = cross(originv1, r.direction);
    float d = 1.0 / dot(r.direction, normal);
    float u = d * dot(-q, v3v1);
    float v = d * dot(q, v2v1);
    float t = d * dot(-normal, originv1);

    if (u < 0.0 || u > 1.0 || v < 0.0 || u + v > 1.0 || t < r.min_t || t > r.max_t) {
        return false;
    }

    info.t = t;
    info.barycentrics = vec3(1 - u - v, u, v);
    info.geometry_normal = normalize(normal);
    info.id = id;

    return true;
}

bool hit_node(ray r, out hit_info info) {
    hit_info temp_info;
    bool hit = false;
    int id = 0;

    while(id != -1) {
        // Leafs
        if (bufs.bvh.nodes[id].primitive_id != -1) {
            // if (hit_sphere(bufs.geometry.spheres[bufs.bvh.nodes[id].primitive_id], r, temp_info)) {
            //     info = temp_info;
            //     r.max_t = temp_info.t;
            //     hit = true;
            // }
            if (hit_triangle(bufs.bvh.nodes[id].primitive_id, r, temp_info)) {
                info = temp_info;
                r.max_t = temp_info.t;
                hit = true;
            }
            id = bufs.bvh.nodes[id].next_id;
        } else if (hit_aabb(bufs.bvh.nodes[id].min, bufs.bvh.nodes[id].max, r)) {
                id++;
        } else {
            id = bufs.bvh.nodes[id].next_id;
        }
    }

    info.point = at(r, info.t);
    return hit;
}
