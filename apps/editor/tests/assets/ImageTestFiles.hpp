#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TechEngineTests {
    struct RgbePixel {
        std::uint8_t red = 0;
        std::uint8_t green = 0;
        std::uint8_t blue = 0;
        std::uint8_t exponent = 0;
    };

    // stb_image decodes a mantissa byte to the middle of its step, and an exponent byte of 0 to 0.
    inline float rgbeChannel(const std::uint8_t mantissa, const std::uint8_t exponent) {
        if (exponent == 0) {
            return 0.0f;
        }
        return (static_cast<float>(mantissa) + 0.5f) * std::ldexp(1.0f, static_cast<int>(exponent) - 136);
    }

    // An uncompressed Radiance file, top row first. The first pixel of a file 8 or more wide must not start with 2, 2, or
    // a reader takes the scanline for run-length encoded.
    inline std::string radianceFile(const std::uint32_t width, const std::uint32_t height, const std::vector<RgbePixel>& pixels) {
        std::string text = "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y " + std::to_string(height) + " +X " + std::to_string(width) + "\n";
        for (const RgbePixel& pixel: pixels) {
            text.push_back(static_cast<char>(pixel.red));
            text.push_back(static_cast<char>(pixel.green));
            text.push_back(static_cast<char>(pixel.blue));
            text.push_back(static_cast<char>(pixel.exponent));
        }
        return text;
    }

    inline RgbePixel gradientPixel(const std::uint32_t x, const std::uint32_t y) {
        return RgbePixel{static_cast<std::uint8_t>(10 + x * 20), static_cast<std::uint8_t>(20 + y * 40), static_cast<std::uint8_t>(1 + (x + y) * 17), static_cast<std::uint8_t>(126 + (x + y) % 5)};
    }

    inline std::string gradientRadianceFile(const std::uint32_t width, const std::uint32_t height) {
        std::vector<RgbePixel> pixels;
        for (std::uint32_t y = 0; y < height; y++) {
            for (std::uint32_t x = 0; x < width; x++) {
                pixels.push_back(gradientPixel(x, y));
            }
        }
        return radianceFile(width, height, pixels);
    }

    // An uncompressed 24-bit TGA with its origin at the top left. A pixel is blue, green, red.
    inline std::string tgaFile(const std::uint16_t width, const std::uint16_t height, const std::vector<std::uint8_t>& bluGreenRedBytes) {
        std::string bytes(18, '\0');
        bytes[2] = 2;
        bytes[12] = static_cast<char>(width & 0xFF);
        bytes[13] = static_cast<char>(width >> 8);
        bytes[14] = static_cast<char>(height & 0xFF);
        bytes[15] = static_cast<char>(height >> 8);
        bytes[16] = 24;
        bytes[17] = 0x20;
        for (const std::uint8_t value: bluGreenRedBytes) {
            bytes.push_back(static_cast<char>(value));
        }
        return bytes;
    }
}
