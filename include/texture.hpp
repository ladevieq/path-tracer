#include <cstdint>

using VkImage = struct VkImage_T*;

struct TextureDesc {
    VkImageType type;
    VkFormat format;
    VkExtent size;
    VkImageUsageFlags usages;
};

class Texture {
  public:
    Texture(const TextureDesc desc)
            : description(desc) {
        create_device_image();
    }

    // Texture(handle image_handle) {
    //     const auto& image = vkrenderer::api.get_image(image_handle);
    //     width = image.size.width;
    //     height = image.size.height;
    //     depth = image.size.depth;

    // device_image = image_handle;
    // }

    // Texture(size_t width, size_t height, size_t depth, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, Sampler* texture_sampler = nullptr)
    //         : width(width), height(height), depth(depth) {
    //     sampler = texture_sampler;
    //     device_image = vkrenderer::api.create_image(
    //         { .width = (uint32_t)width, .height = (uint32_t)height, .depth = 1 },
    //         format,
    //         VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    // }

    void update(void* new_data) {
        if (data == nullptr) {
            free(data);
        }
        data = new_data;

        vkrenderer::queue_image_update(this);
    }

    ~Texture() {
        vkrenderer::api.destroy_image(device_image);

        if (data == nullptr) {
            free(data);
        }
    }

    [[nodiscard]] size_t size() const {
        const auto& image = vkrenderer::api.get_image(device_image);
        return width * height * depth * pixel_size(image.format);
    }

    static size_t pixel_size(VkFormat format) {
        switch (format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 4;
        default:
            return 0;
        }
    }

    // Sampler* sampler = nullptr;

  private:
    void create_device_image();

    void* data = nullptr;

    size_t width = 0;
    size_t height = 0;
    size_t depth = 0;

    TextureDesc description;

    VmaAllocationInfo alloc_info;
    VkImage device_image;
};
