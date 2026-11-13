#include <assets/Rgb9E5.hpp>

#include <algorithm>
#include <cmath>

namespace TechEngine {
    static constexpr int MANTISSA_BITS = 9;
    static constexpr int EXPONENT_BIAS = 15;
    static constexpr std::uint32_t MANTISSA_MASK = (1u << MANTISSA_BITS) - 1;
    static constexpr std::uint32_t MAX_MANTISSA = MANTISSA_MASK;
    static constexpr int EXPONENT_SHIFT = 27;

    std::uint32_t Rgb9E5::pack(const float red, const float green, const float blue) {
        const float clampedRed = clampChannel(red);
        const float clampedGreen = clampChannel(green);
        const float clampedBlue = clampChannel(blue);
        const float largest = std::max({clampedRed, clampedGreen, clampedBlue});

        int exponent = std::max(-EXPONENT_BIAS - 1, std::ilogb(largest)) + 1 + EXPONENT_BIAS;
        float denominator = std::exp2(static_cast<float>(exponent - EXPONENT_BIAS - MANTISSA_BITS));
        if (static_cast<std::uint32_t>(std::floor(largest / denominator + 0.5f)) == MAX_MANTISSA + 1) {
            denominator *= 2.0f;
            exponent++;
        }

        const std::uint32_t redMantissa = static_cast<std::uint32_t>(std::floor(clampedRed / denominator + 0.5f));
        const std::uint32_t greenMantissa = static_cast<std::uint32_t>(std::floor(clampedGreen / denominator + 0.5f));
        const std::uint32_t blueMantissa = static_cast<std::uint32_t>(std::floor(clampedBlue / denominator + 0.5f));
        return (static_cast<std::uint32_t>(exponent) << EXPONENT_SHIFT) | (blueMantissa << (2 * MANTISSA_BITS)) | (greenMantissa << MANTISSA_BITS) | redMantissa;
    }

    void Rgb9E5::unpack(const std::uint32_t packed, float& red, float& green, float& blue) {
        const int exponent = static_cast<int>(packed >> EXPONENT_SHIFT);
        const float scale = std::exp2(static_cast<float>(exponent - EXPONENT_BIAS - MANTISSA_BITS));
        red = static_cast<float>(packed & MANTISSA_MASK) * scale;
        green = static_cast<float>((packed >> MANTISSA_BITS) & MANTISSA_MASK) * scale;
        blue = static_cast<float>((packed >> (2 * MANTISSA_BITS)) & MANTISSA_MASK) * scale;
    }

    float Rgb9E5::clampChannel(const float value) {
        if (!(value > 0.0f)) {
            return 0.0f;
        }
        return std::min(value, MAX_VALUE);
    }
}
