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
        bvh_node(sphere* spheres, size_t start, size_t end, bvh_node* root_node, int32_t id) {
            left_node_id = id * 2;
            right_node_id = id * 2 + 1;

            bvh_node left, right;

            // int axis = randi(0, 2);
            // auto comparator = (axis == 0) ? box_x_comparator : (axis == 1) ? box_y_comparator : box_z_comparator;

            size_t span = end - start;

            if (span == 1) {
                root_node[left_node_id - 1] = root_node[right_node_id - 1] = { spheres[start], (int32_t)start };
            } else if (span == 2) {
                // if (comparator(spheres[start], spheres[start + 1]) {
                    root_node[left_node_id - 1] = { spheres[start], (int32_t)start };
                    root_node[right_node_id - 1] = { spheres[start], (int32_t)start + 1 };
                // } else {
                //     root_node[left_node_id - 1] = { spheres[start + 1], start + 1 };
                //     root_node[right_node_id - 1] = { spheres[start], start };
                // }
            } else {
                uint32_t mid = start + span / 2;
                root_node[left_node_id - 1] = { spheres, start, mid, root_node, left_node_id };
                root_node[right_node_id - 1] = { spheres, mid, end, root_node, right_node_id };
            }

            bounding_box = aabb::surrounding_box(root_node[left_node_id - 1].bounding_box, root_node[right_node_id -1].bounding_box);
        }

        aabb bounding_box;

        // Store sphere id ?
        int32_t left_node_id    = -1;
        int32_t right_node_id   = -1;

        int32_t sphere_id       = -1;
        uint32_t padding;
};

#endif // !__BVH_NODE_HPP_
