#ifndef __BVH_NODE_HPP_
#define __BVH_NODE_HPP_

#include <vector>
#include <algorithm>
#include <fstream>

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
        boxes = std::vector<aabb>(spheres_count);

        for (uint32_t sphere_id = 0; sphere_id < spheres_count; sphere_id++) {
            boxes[sphere_id] = spheres[sphere_id].bounding_box;
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
        // preorder(nodes[0].left_id, nodes[0].sphere_id);
        traverse(nodes[0].left_id);

        std::cout << "\t 0";
        // preorder(nodes[0].left_id + 1, nodes[0].sphere_id);
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
            global_box = aabb::surrounding_box(global_box, boxes[box_id]);
        }

        return global_box;
    }

    void subdivide(uint32_t parent_id, uint32_t begin, uint32_t end) {
        if ((end - begin) == 2) {
            auto left_id = ++next_node_id;
            auto right_id = ++next_node_id;

            nodes[parent_id].left_id = left_id;

            nodes[left_id] = bvh_node();
            nodes[right_id] = bvh_node();

            nodes[left_id].bounding_box = boxes[begin];
            nodes[right_id].bounding_box = boxes[begin + 1];

            nodes[left_id].sphere_id = begin;
            nodes[right_id].sphere_id = begin + 1;
        } else if ((end - begin) == 1) {
            nodes[parent_id].bounding_box = boxes[begin];
            nodes[parent_id].sphere_id = begin;
        } else {
            auto left_id = ++next_node_id;
            auto right_id = ++next_node_id;

            nodes[parent_id].left_id = left_id;

            nodes[left_id] = bvh_node();
            nodes[right_id] = bvh_node();

            nodes[left_id].bounding_box = compute_bounds(begin, begin + ((end - begin) / 2) + 1);
            nodes[right_id].bounding_box = compute_bounds(begin + ((end - begin) / 2) + 1, end);

            subdivide(left_id, begin, begin + ((end - begin) / 2) + 1);
            subdivide(right_id, begin + ((end - begin) / 2) + 1, end);
        }
    }

public:
    std::vector<bvh_node> nodes;
    std::vector<aabb> boxes;
    uint32_t next_node_id = 0;
};

#endif // !__BVH_NODE_HPP_
