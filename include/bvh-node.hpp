#ifndef __BVH_NODE_HPP_
#define __BVH_NODE_HPP_

#include <vector>
#include <algorithm>

#include "aabb.hpp"
#include "sphere.hpp"

class bvh_node {
    public:
        bvh_node() = default;
        bvh_node(sphere& sphere, int32_t sphere_id): bounding_box(sphere.bounding_box), sphere_id(sphere_id){}

        aabb bounding_box;

        int32_t left_id    = -1;
        int32_t sphere_id   = -1;
        uint32_t padding[2];
};

class bvh {
public: 
    void build(sphere* spheres, uint32_t spheres_count) {
        nodes = std::vector<bvh_node>(2 * spheres_count - 1);
        leafs = std::vector<bvh_node>(spheres_count);
        this->spheres = spheres;

        for (uint32_t id = 0; id < spheres_count; id++) {
            leafs[id].bounding_box = spheres[id].bounding_box;
            leafs[id].sphere_id = id;
        }

        nodes[0] = bvh_node();

        nodes[0].bounding_box = compute_bounds(0, spheres_count);
        subdivide(0, 0, spheres_count);
    }

    //-------------------------
    // BVH exporter for graphviz
    //-------------------------
    void exporter() {
        std::cout << "digraph bvh {" << std::endl;

        std::cout << "\t 0";
        traverse(nodes[0].left_id);

        std::cout << "\t 0";
        traverse(nodes[0].left_id + 1);

        std::cout << "}" << std::endl;
    }

    bool traverse(int32_t id) {
        if (id <= 0) {
            std::cout << ";" << std::endl;
            return false;
        }

        std::cout << " -> " << id;
        if (traverse(nodes[id].left_id)) {
            std::cout << "\t " << id;
            traverse(nodes[id].left_id + 1);
        }

        return true;
    }

private:

    aabb compute_bounds(uint32_t begin, uint32_t end) {
        aabb global_box = aabb();

        for (uint32_t box_id = begin; box_id < end; box_id++) {
            auto leaf = leafs[box_id];
            global_box = aabb::surrounding_box(global_box, spheres[leaf.sphere_id].bounding_box);
        }

        return global_box;
    }

    void subdivide(uint32_t parent_id, uint32_t begin, uint32_t end) {
        size_t count = end - begin;
         if (count == 1) {
            nodes[parent_id] = leafs[begin];
            return;
        }

        auto left_id = ++next_node_id;
        auto right_id = ++next_node_id;

        nodes[parent_id].left_id = left_id;

        if (count == 2) {
            nodes[left_id] = leafs[begin];
            nodes[right_id] = leafs[begin + 1];
        } else {
            auto bound = nodes[parent_id].bounding_box;
            int split_axis = 0;
            float size = 0;

            // Find largest axis
            for (size_t id = 0; id < 3; id++) {
                auto current_size = bound.maximum[id] - bound.minimum[id];
                if (current_size > size) {
                    size = current_size;
                    split_axis = id;
                }
            }

            std::sort(leafs.begin() + begin, leafs.begin() + end,
                [&](const bvh_node& a, const bvh_node& b) {
                    return spheres[a.sphere_id].center[split_axis] < spheres[b.sphere_id].center[split_axis];
                }
            );

            float split_axis_pos = (bound.maximum.x + bound.minimum.x) * 0.5f;
            int mid = end - 1;

            for (size_t i = begin + 1; i < end; i++) {
                auto leaf = leafs[i];
                if (spheres[leaf.sphere_id].center[split_axis] >= split_axis_pos) {
                    mid = i;
                    break;
                }
            }

            nodes[left_id].bounding_box = compute_bounds(begin, mid);
            nodes[right_id].bounding_box = compute_bounds(mid, end);

            subdivide(left_id, begin, mid);
            subdivide(right_id, mid, end);
        }
    }

public:
    std::vector<bvh_node> nodes;
    std::vector<bvh_node> leafs;
    sphere* spheres;
    uint32_t next_node_id = 0;
};

#endif // !__BVH_NODE_HPP_
