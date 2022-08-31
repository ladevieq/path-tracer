#pragma once

#include <vector>

#include "aabb.hpp"
// #include "sphere.hpp"
#include "triangle.hpp"

struct bvh_node {
    aabb bounding_box;

    int32_t next_id    = -1;
    int32_t primitive_id   = -1;
    uint32_t padding[2];
};

struct packed_bvh_node {
    float min[3];
    int32_t next_id = -1;
    float max[3];
    int32_t primitive_id = -1;
};

struct temp_node : public bvh_node {
    int32_t left_id = -1;
    int32_t df_id;      // depth firt id
};

class bvh {
public: 
    // bvh(std::vector<sphere>& spheres, std::vector<packed_bvh_node>& packed_nodes);

    bvh(std::vector<triangle>& triangles, std::vector<packed_bvh_node>& packed_nodes);

    void exporter();

    static bool traverse_depth_first(std::vector<temp_node>& nodes, int32_t id);

    static int32_t traverse_depth_first_ordered(std::vector<bvh_node>& nodes, int32_t parent_id, int32_t id);

    std::vector<temp_node> temp_nodes;
    std::vector<temp_node> leafs;

    // std::vector<sphere>& spheres;
    std::vector<triangle>& triangles;

    uint32_t next_node_id = 0;

private:

    static constexpr uint32_t buckets_count = 12;

    aabb compute_bounds(uint32_t begin, uint32_t end);

    void subdivide(uint32_t parent_id, uint32_t begin, uint32_t end);

    void set_depth_first_order();

    void depth_first_order(int32_t id, int32_t next_id, int32_t &new_id);
};
