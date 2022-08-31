#include "mesh.hpp"

static std::vector<Mesh> meshes {};

void Mesh::add_submesh(const submesh& submesh) {
    submeshes.emplace_back(
        submesh_info {
            (uint32_t)vertex_count,
            (uint32_t)index_count,
            submesh.index_count / 3,
            0
        }
    );

    {
        auto indices_off = indices.size();
        indices.resize(indices_off + submesh.index_count);
        std::memcpy(&indices[indices_off], submesh.indices.data, submesh.indices.length);
    }

    for (uint32_t attribute_index { ATTRIBUTE_TYPE::POSITION }; attribute_index < ATTRIBUTE_TYPE::MAX_ATTRIBUTE; attribute_index++) {
        const auto& submesh_attribute = submesh.attributes[attribute_index];
        if (submesh_attribute.data == nullptr) continue;

        auto& mesh_attribute = attributes[attribute_index];
        auto attribute_off = mesh_attribute.size();
        auto element_components = attribute_components(static_cast<ATTRIBUTE_TYPE>(attribute_index));
        auto elements_count = submesh.vertex_count * element_components;

        mesh_attribute.resize(attribute_off + elements_count);
        std::memcpy(&mesh_attribute[attribute_off], submesh_attribute.data, submesh_attribute.length);
    }

    vertex_count += submesh.vertex_count;
    index_count += submesh.index_count;
}

[[nodiscard]] Mesh::mesh_instance Mesh::instantiate () const {
    return {
        *this
    };
}

void Mesh::register_mesh(const Mesh& mesh) {
    meshes.emplace_back(mesh);
}
