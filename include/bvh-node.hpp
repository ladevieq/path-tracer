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

        int32_t next_id    = -1;
        int32_t sphere_id   = -1;
        uint32_t padding[2];
};

struct temp_node : public bvh_node {
    int32_t left_id;
    int32_t df_id;
};

class bvh {
public: 
    void build(sphere* spheres, uint32_t spheres_count) {
        nodes = std::vector<bvh_node>(2 * spheres_count - 1);
        temp_nodes = std::vector<temp_node>(2 * spheres_count - 1);
        leafs = std::vector<temp_node>(spheres_count);
        this->spheres = spheres;

        for (uint32_t id = 0; id < spheres_count; id++) {
            leafs[id].bounding_box = spheres[id].bounding_box;
            leafs[id].sphere_id = id;
        }

        temp_nodes[0] = temp_node();

        temp_nodes[0].bounding_box = compute_bounds(0, spheres_count);
        subdivide(0, 0, spheres_count);

        set_depth_first_order();

        for (uint32_t id = 0; id < temp_nodes.size(); id++) {
            auto old_node = temp_nodes[id];
            auto &node = nodes[old_node.df_id];

            node.sphere_id = old_node.sphere_id;
            node.bounding_box = old_node.bounding_box;

            if (old_node.next_id != -1) {
                node.next_id = temp_nodes[old_node.next_id].df_id;
            }
        }

        // exporter();
    }

    //-------------------------
    // BVH exporter for graphviz
    //-------------------------
    void exporter() {
        std::cout << "digraph bvh {" << std::endl;

        traverse_depth_first(temp_nodes, 0);

        // traverse_depth_first_ordered(-1, 0);

        std::cout << "}" << std::endl;
    }

    bool traverse_depth_first(std::vector<temp_node>& nodes, int32_t id) {
        if (id <= 0) {
            std::cout << ";" << std::endl;
            return false;
        }

        std::cout << " -> " << id;
        if (traverse_depth_first(nodes, nodes[id].left_id)) {
            std::cout << "\t " << id;
            traverse_depth_first(nodes, nodes[id].left_id + 1);
        }

        return true;
    }

    int32_t traverse_depth_first_ordered(std::vector<bvh_node>& nodes, int32_t parent_id, int32_t id) {
        if (id == -1) {
            return -1;
        }

        if (nodes[id].sphere_id == -1) {
            int32_t left = id + 1;
            int32_t next_id;

            std::cout << "\t " << parent_id << " -> " << id << ";\n";
            next_id = traverse_depth_first_ordered(nodes, id, left);

            if (next_id != -1) {
                return traverse_depth_first_ordered(nodes, id, next_id);
            }
        } else if (nodes[id].sphere_id != -1 && nodes[id].next_id != -1) {
            std::cout << "\t " << parent_id << " -> " << id << ";\n";
            return nodes[id].next_id;
        } else {
            std::cout << "\t " << parent_id << " -> " << id << ";\n";
        }

        return -1;
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

    int32_t find_split_axis(aabb bounding_box) {
        int32_t split_axis = 0;
        float size = 0;

        // Find largest axis
        for (size_t id = 0; id < 3; id++) {
            auto current_size = bounding_box.maximum[id] - bounding_box.minimum[id];
            if (current_size > size) {
                size = current_size;
                split_axis = id;
            }
        }

        return split_axis;
    }

    void subdivide(uint32_t parent_id, uint32_t begin, uint32_t end) {
        size_t count = end - begin;
         if (count == 1) {
            temp_nodes[parent_id] = leafs[begin];
            return;
        }

        auto left_id = ++next_node_id;
        auto right_id = ++next_node_id;

        temp_nodes[parent_id].left_id = left_id;

        if (count == 2) {
            temp_nodes[left_id] = leafs[begin];
            temp_nodes[right_id] = leafs[begin + 1];
        } else {
            int32_t split_axis = find_split_axis(temp_nodes[parent_id].bounding_box);

            std::sort(leafs.begin() + begin, leafs.begin() + end,
                [&](const bvh_node& a, const bvh_node& b) {
                    return spheres[a.sphere_id].center[split_axis] < spheres[b.sphere_id].center[split_axis];
                }
            );

            temp_nodes[left_id].bounding_box = compute_bounds(begin, begin + (count / 2));
            temp_nodes[right_id].bounding_box = compute_bounds(begin + (count / 2), end);

            subdivide(left_id, begin, begin + (count / 2));
            subdivide(right_id, begin + (count / 2), end);
        }
    }

    void set_depth_first_order() {
        int32_t df_index = 0;
        depth_first_order(0, -1, df_index);
    }


    void depth_first_order(int32_t id, int32_t next_id, int32_t &new_id) {
        auto &node = temp_nodes[id];
        node.next_id = next_id;
        node.df_id = new_id++;

        if (node.left_id > 0) {
            depth_first_order(node.left_id, node.left_id + 1, new_id);
            depth_first_order(node.left_id + 1, next_id, new_id);
        }
    }

public:
    std::vector<bvh_node> nodes;
    std::vector<temp_node> temp_nodes;
    std::vector<temp_node> leafs;
    sphere* spheres;
    uint32_t next_node_id = 0;
};

#endif // !__BVH_NODE_HPP_
