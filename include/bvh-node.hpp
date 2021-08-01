#ifndef __BVH_NODE_HPP_
#define __BVH_NODE_HPP_

#include <vector>
#include <algorithm>

#include "aabb.hpp"
#include "sphere.hpp"
#include "triangle.hpp"

class bvh_node {
    public:
        bvh_node() = default;
        bvh_node(sphere& sphere, int32_t sphere_id): bounding_box(sphere.bounding_box), primitive_id(sphere_id){}

        aabb bounding_box;

        int32_t next_id    = -1;
        int32_t primitive_id   = -1;
        uint32_t padding[2];
};

struct temp_node : public bvh_node {
    int32_t left_id;
    int32_t df_id;
};

class bvh {
public: 
    // bvh(std::vector<sphere>& spheres, std::vector<bvh_node>& nodes)
    //     :spheres(spheres), nodes(nodes) {
    //     nodes = std::vector<bvh_node>(2 * spheres.size() - 1);
    //     temp_nodes = std::vector<temp_node>(2 * spheres.size() - 1);
    //     leafs = std::vector<temp_node>(spheres.size());

    //     for (uint32_t id = 0; id < spheres.size(); id++) {
    //         leafs[id].bounding_box = spheres[id].bounding_box;
    //         leafs[id].primitive_id = id;
    //     }

    //     temp_nodes[0] = temp_node();

    //     temp_nodes[0].bounding_box = compute_bounds(0, spheres.size());
    //     subdivide(0, 0, spheres.size());

    //     set_depth_first_order();

    //     for (uint32_t id = 0; id < temp_nodes.size(); id++) {
    //         auto old_node = temp_nodes[id];
    //         auto &node = nodes[old_node.df_id];

    //         node.primitive_id = old_node.primitive_id;
    //         node.bounding_box = old_node.bounding_box;

    //         if (old_node.next_id != -1) {
    //             node.next_id = temp_nodes[old_node.next_id].df_id;
    //         }
    //     }

    //     // exporter();
    // }

    bvh(std::vector<triangle>& triangles, std::vector<bvh_node>& nodes)
        :triangles(triangles), nodes(nodes) {
        nodes.resize(2 * triangles.size() - 1);
        temp_nodes = std::vector<temp_node>(2 * triangles.size() - 1);
        leafs = std::vector<temp_node>(triangles.size());

        for (uint32_t id = 0; id < triangles.size(); id++) {
            leafs[id].bounding_box = triangles[id].bounding_box;
            leafs[id].primitive_id = id;
        }

        temp_nodes[0] = temp_node();

        temp_nodes[0].bounding_box = compute_bounds(0, triangles.size());
        subdivide(0, 0, triangles.size());

        set_depth_first_order();

        for (uint32_t id = 0; id < temp_nodes.size(); id++) {
            auto old_node = temp_nodes[id];
            auto &node = nodes[old_node.df_id];

            node.primitive_id = old_node.primitive_id;
            node.bounding_box = old_node.bounding_box;

            if (old_node.next_id != -1) {
                node.next_id = temp_nodes[old_node.next_id].df_id;
            }
        }

        // TODO: Pack nodes

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

        if (nodes[id].primitive_id == -1) {
            int32_t left = id + 1;
            int32_t next_id;

            std::cout << "\t " << parent_id << " -> " << id << ";\n";
            next_id = traverse_depth_first_ordered(nodes, id, left);

            if (next_id != -1) {
                return traverse_depth_first_ordered(nodes, id, next_id);
            }
        } else if (nodes[id].primitive_id != -1 && nodes[id].next_id != -1) {
            std::cout << "\t " << parent_id << " -> " << id << ";\n";
            return nodes[id].next_id;
        } else {
            std::cout << "\t " << parent_id << " -> " << id << ";\n";
        }

        return -1;
    }

    std::vector<bvh_node>& nodes;
    std::vector<temp_node> temp_nodes;
    std::vector<temp_node> leafs;

    // std::vector<sphere>& spheres;
    std::vector<triangle>& triangles;

    uint32_t next_node_id = 0;

private:

    static constexpr uint32_t buckets_count = 12;

    aabb compute_bounds(uint32_t begin, uint32_t end) {
        aabb global_box = aabb();

        for (uint32_t box_id = begin; box_id < end; box_id++) {
            auto leaf = leafs[box_id];
            // global_box = aabb::surrounding_box(global_box, spheres[leaf.primitive_id].bounding_box);
            global_box.union_with(triangles[leaf.primitive_id].bounding_box);
        }

        return global_box;
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
            auto parent_bb = temp_nodes[parent_id].bounding_box;
            int32_t split_axis = parent_bb.maximum_axis();

            if (count <= 4) {
                // Subdivide the node by splitting the set in two equal parts
                std::sort(leafs.begin() + begin, leafs.begin() + end,
                    // [&](const bvh_node& a, const bvh_node& b) {
                    //     return spheres[a.primitive_id].center[split_axis] < spheres[b.primitive_id].center[split_axis];
                    // }
                    [&](const bvh_node& a, const bvh_node& b) {
                        return triangles[a.primitive_id].center()[split_axis] < triangles[b.primitive_id].center()[split_axis];
                    }
                );

                temp_nodes[left_id].bounding_box = compute_bounds(begin, begin + (count / 2));
                temp_nodes[right_id].bounding_box = compute_bounds(begin + (count / 2), end);

                subdivide(left_id, begin, begin + (count / 2));
                subdivide(right_id, begin + (count / 2), end);
            } else {
                // Subdivide based on SAH
                struct Bucket {
                    uint32_t count = 0;
                    aabb bb;
                };
                Bucket buckets[buckets_count];

                // Split the set in N spatial buckets
                // Simplify the optimal split axis search to just an axis between each bucket
                aabb centroid_bounds;

                for (size_t i = begin; i < end; i++) {
                    auto primitive = triangles[leafs[i].primitive_id];
                    centroid_bounds.union_with(primitive.center());
                }

                float split_axis_size = centroid_bounds.maximum[split_axis] - centroid_bounds.minimum[split_axis];
                for (size_t i = begin; i < end; i++) {
                    auto primitive = triangles[leafs[i].primitive_id];
                    auto normalized_offset = (primitive.center()[split_axis] - centroid_bounds.minimum[split_axis]) / split_axis_size;
                    uint32_t bucket_index = normalized_offset * buckets_count;
                    if (bucket_index == buckets_count) {
                        bucket_index -= 1;
                    }

                    buckets[bucket_index].count++;
                    buckets[bucket_index].bb.union_with(primitive.bounding_box);
                }

                // Compute each potential split axis cost
                float buckets_cost[buckets_count - 1] = { 0.f };
                for (size_t bucket_index = 0; bucket_index < buckets_count - 1; bucket_index++) {
                    aabb right_side_bb, left_side_bb;
                    float right_side_count = 0, left_side_count = 0;

                    for (size_t i = 0; i <= bucket_index; i++) {
                        left_side_bb.union_with(buckets[i].bb);
                        left_side_count += buckets[i].count;
                    }

                    for (size_t i = bucket_index + 1; i < buckets_count; i++) {
                        right_side_bb.union_with(buckets[i].bb);
                        right_side_count += buckets[i].count;
                    }

                    float left = left_side_count * left_side_bb.surface_area();
                    float right = right_side_count * right_side_bb.surface_area();
                    buckets_cost[bucket_index] = 0.125f + (left + right) / parent_bb.surface_area();
                }

                float min_cost = buckets_cost[0];
                uint32_t min_cost_bucket = 0;
                for (size_t bucket_index = 1; bucket_index < buckets_count - 1; bucket_index++) {
                    if (buckets_cost[bucket_index] < min_cost) {
                        min_cost = buckets_cost[bucket_index];
                        min_cost_bucket = bucket_index;
                    }
                }

                uint32_t middle_primitive_index = buckets[0].count;
                for (size_t bucket_index = 1; bucket_index <= min_cost_bucket; bucket_index++) {
                    middle_primitive_index += buckets[bucket_index].count;
                }

                temp_nodes[left_id].bounding_box = compute_bounds(begin, begin + middle_primitive_index);
                temp_nodes[right_id].bounding_box = compute_bounds(begin + middle_primitive_index, end);

                subdivide(left_id, begin, begin + middle_primitive_index);
                subdivide(right_id, begin + middle_primitive_index, end);
            }
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
};

#endif // !__BVH_NODE_HPP_
