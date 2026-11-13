#pragma once

#include <cstdint>

namespace TechEngine {
    class Rgb9E5 {
    public:
        // The largest value each channel keeps: (511 / 512) * 2^16.
        static constexpr float MAX_VALUE = 65408.0f;

        // Negative and NaN channels become 0, and a channel above MAX_VALUE becomes MAX_VALUE. The result is red in
        // bits 0-8, green in 9-17, blue in 18-26 and the biased exponent in 27-31, as GL_RGB9_E5 stores it.
        static std::uint32_t pack(float red, float green, float blue);

        static void unpack(std::uint32_t packed, float& red, float& green, float& blue);
    };
}
