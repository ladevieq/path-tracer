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
        // bvh_node(sphere* spheres, size_t start, size_t end, bvh_node* root_node, int32_t id) {
        //     left_node_id = id * 2;
        //     right_node_id = id * 2 + 1;

        //     bvh_node left, right;

        //     // int axis = randi(0, 2);
        //     // auto comparator = (axis == 0) ? box_x_comparator : (axis == 1) ? box_y_comparator : box_z_comparator;

        //     size_t span = end - start;

        //     if (span == 1) {
        //         root_node[left_node_id - 1] = root_node[right_node_id - 1] = { spheres[start], (int32_t)start };
        //     } else if (span == 2) {
        //         // if (comparator(spheres[start], spheres[start + 1]) {
        //             root_node[left_node_id - 1] = { spheres[start], (int32_t)start };
        //             root_node[right_node_id - 1] = { spheres[start], (int32_t)start + 1 };
        //         // } else {
        //         //     root_node[left_node_id - 1] = { spheres[start + 1], start + 1 };
        //         //     root_node[right_node_id - 1] = { spheres[start], start };
        //         // }
        //     } else {
        //         uint32_t mid = start + span / 2;
        //         root_node[left_node_id - 1] = { spheres, start, mid, root_node, left_node_id };
        //         root_node[right_node_id - 1] = { spheres, mid, end, root_node, right_node_id };
        //     }

        //     bounding_box = aabb::surrounding_box(root_node[left_node_id - 1].bounding_box, root_node[right_node_id -1].bounding_box);
        // }

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

    void preorder(int32_t id, int32_t sphere_id) {
        if (id <= 0) {
            return;
        }

        if (nodes[id].left_id > -1) {
            std::cout << " -> " << id;
            preorder(nodes[id].left_id, nodes[id].sphere_id);

            std::cout << "\t " << id;
            if (nodes[nodes[id].left_id + 1].sphere_id == -1) {
                preorder(nodes[id].left_id + 1, nodes[id].sphere_id);
            } else {
                std::cout << " -> sphere_" << nodes[nodes[id].left_id + 1].sphere_id << ";" << std::endl;
            }
        } else if (nodes[id].sphere_id > -1) {
            std::cout << " -> sphere_" << nodes[id].sphere_id << ";" << std::endl;
        }
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
