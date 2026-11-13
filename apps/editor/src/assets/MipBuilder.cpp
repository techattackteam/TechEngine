#include <assets/MipBuilder.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace TechEngine {
    static constexpr std::size_t CHANNELS = 4;
    static constexpr float TAP_WEIGHT = 0.25f;
    static constexpr float MIN_NORMAL_LENGTH_SQUARED = 1.0e-12f;

    void MipBuilder::build(const FloatImage& source, std::vector<FloatImage>& out) {
        out.clear();
        if (source.width == 0 || source.height == 0) {
            return;
        }
        std::uint32_t width = source.width;
        std::uint32_t height = source.height;
        while (width > 1 || height > 1) {
            const FloatImage& above = out.empty() ? source : out.back();
            FloatImage level;
            downsample(above, level);
            width = level.width;
            height = level.height;
            out.push_back(std::move(level));
        }
    }

    void MipBuilder::buildNormalMap(const FloatImage& source, std::vector<FloatImage>& out) {
        out.clear();
        if (source.width == 0 || source.height == 0) {
            return;
        }
        std::uint32_t width = source.width;
        std::uint32_t height = source.height;
        while (width > 1 || height > 1) {
            const FloatImage& above = out.empty() ? source : out.back();
            FloatImage level;
            downsample(above, level);
            renormalize(level);
            width = level.width;
            height = level.height;
            out.push_back(std::move(level));
        }
    }

    void MipBuilder::renormalize(FloatImage& image) {
        for (std::size_t pixel = 0; pixel < image.pixels.size(); pixel += CHANNELS) {
            float* texel = &image.pixels[pixel];
            const float lengthSquared = texel[0] * texel[0] + texel[1] * texel[1] + texel[2] * texel[2];
            if (lengthSquared < MIN_NORMAL_LENGTH_SQUARED) {
                texel[0] = 0.0f;
                texel[1] = 0.0f;
                texel[2] = 1.0f;
                continue;
            }
            const float inverseLength = 1.0f / std::sqrt(lengthSquared);
            texel[0] *= inverseLength;
            texel[1] *= inverseLength;
            texel[2] *= inverseLength;
        }
    }

    void MipBuilder::downsample(const FloatImage& above, FloatImage& out) {
        out.width = std::max<std::uint32_t>(above.width / 2, 1);
        out.height = std::max<std::uint32_t>(above.height / 2, 1);
        out.pixels.assign(static_cast<std::size_t>(out.width) * out.height * CHANNELS, 0.0f);

        for (std::uint32_t y = 0; y < out.height; y++) {
            for (std::uint32_t x = 0; x < out.width; x++) {
                float* destination = &out.pixels[(static_cast<std::size_t>(y) * out.width + x) * CHANNELS];
                for (std::uint32_t tapY = 0; tapY < 2; tapY++) {
                    const std::uint32_t sourceY = std::min(y * 2 + tapY, above.height - 1);
                    for (std::uint32_t tapX = 0; tapX < 2; tapX++) {
                        const std::uint32_t sourceX = std::min(x * 2 + tapX, above.width - 1);
                        const float* tap = &above.pixels[(static_cast<std::size_t>(sourceY) * above.width + sourceX) * CHANNELS];
                        for (std::size_t channel = 0; channel < CHANNELS; channel++) {
                            destination[channel] += tap[channel] * TAP_WEIGHT;
                        }
                    }
                }
            }
        }
    }
}
