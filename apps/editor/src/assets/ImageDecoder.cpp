#include <assets/ImageDecoder.hpp>
#include <project/ProjectError.hpp>

#include <stb_image.h>

#include <cstddef>
#include <limits>

namespace TechEngine {
    static constexpr int RGBA_CHANNELS = 4;

    static bool hasValidLength(const std::span<const std::byte> bytes) {
        return !bytes.empty() && bytes.size() <= static_cast<std::size_t>(std::numeric_limits<int>::max());
    }

    std::error_code ImageDecoder::decodeFloat(const std::span<const std::byte> bytes, FloatImage& out) {
        if (!hasValidLength(bytes)) {
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
        if (!hasValidLength(bytes)) {
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
}
