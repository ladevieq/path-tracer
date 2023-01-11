#include "resource-manager.hpp"

const auto ResourceManager::resource_manager = ResourceManager();

Handle<Texture> ResourceManager::create_texture(const TextureDesc &desc) {
    textures.add({});
    return { 0 };
}
