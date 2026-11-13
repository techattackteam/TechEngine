#include <assets/ImageDecoder.hpp>
#include <project/ProjectError.hpp>

#include <stb_image.h>

#include <cstddef>
#include <limits>

namespace TechEngine {
    static constexpr int RGBA_CHANNELS = 4;

    std::error_code ImageDecoder::decodeFloat(const std::span<const std::byte> bytes, FloatImage& out) {
        if (!isDecodable(bytes)) {
            return ProjectError::ImageUnreadable;
        }
        int width = 0;
        int height = 0;
        int channels = 0;
        float* decoded = stbi_loadf_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels, RGBA_CHANNELS);
        if (decoded == nullptr || width <= 0 || height <= 0) {
            stbi_image_free(decoded);
            return ProjectError::ImageUnreadable;
        }

        out.width = static_cast<std::uint32_t>(width);
        out.height = static_cast<std::uint32_t>(height);
        out.pixels.assign(decoded, decoded + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * RGBA_CHANNELS);
        stbi_image_free(decoded);
        return {};
    }

    std::error_code ImageDecoder::decodeBytes(const std::span<const std::byte> bytes, ByteImage& out) {
        if (!isDecodable(bytes)) {
            return ProjectError::ImageUnreadable;
        }
        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels, RGBA_CHANNELS);
        if (decoded == nullptr || width <= 0 || height <= 0) {
            stbi_image_free(decoded);
            return ProjectError::ImageUnreadable;
        }

        out.width = static_cast<std::uint32_t>(width);
        out.height = static_cast<std::uint32_t>(height);
        out.pixels.assign(decoded, decoded + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * RGBA_CHANNELS);
        stbi_image_free(decoded);
        return {};
    }

    bool ImageDecoder::isDecodable(const std::span<const std::byte> bytes) {
        if (bytes.empty() || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return false;
        }
        int width = 0;
        int height = 0;
        int channels = 0;
        if (stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(bytes.data()), static_cast<int>(bytes.size()), &width, &height, &channels) == 0) {
            return false;
        }
        return width > 0 && height > 0 && static_cast<std::uint32_t>(width) <= MAX_IMAGE_SIDE && static_cast<std::uint32_t>(height) <= MAX_IMAGE_SIDE;
    }
}
