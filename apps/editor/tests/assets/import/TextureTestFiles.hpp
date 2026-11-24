#pragma once

#include <TechEngine/core/texture/Texture.hpp>

#include <bcdec.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngineTests {
    inline void appendBigEndianU32(std::string& out, const std::uint32_t value) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            out.push_back(static_cast<char>((value >> shift) & 0xFFu));
        }
    }

    inline void appendLittleEndianU32(std::string& out, const std::uint32_t value) {
        for (std::uint32_t shift = 0; shift < 32; shift += 8) {
            out.push_back(static_cast<char>((value >> shift) & 0xFFu));
        }
    }

    inline std::uint32_t crc32(const std::string_view bytes) {
        std::uint32_t crc = 0xFFFFFFFFu;
        for (const char byte: bytes) {
            crc ^= static_cast<std::uint8_t>(byte);
            for (int bit = 0; bit < 8; bit++) {
                crc = (crc & 1u) != 0 ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
            }
        }
        return crc ^ 0xFFFFFFFFu;
    }

    inline void appendPngChunk(std::string& out, const std::string_view type, const std::string& data) {
        appendBigEndianU32(out, static_cast<std::uint32_t>(data.size()));
        const std::string typed = std::string(type) + data;
        out += typed;
        appendBigEndianU32(out, crc32(typed));
    }

    // An 8-bit RGBA PNG, top row first, whose zlib stream holds stored (uncompressed) deflate blocks.
    inline std::string pngFile(const std::uint32_t width, const std::uint32_t height, const std::vector<std::uint8_t>& rgba) {
        std::string raw;
        for (std::uint32_t y = 0; y < height; y++) {
            raw.push_back('\0');
            for (std::uint32_t x = 0; x < width * 4; x++) {
                raw.push_back(static_cast<char>(rgba[(static_cast<std::size_t>(y) * width * 4) + x]));
            }
        }

        std::string zlib = "\x78\x01";
        std::size_t offset = 0;
        do {
            const std::size_t length = std::min<std::size_t>(raw.size() - offset, 65535);
            const bool last = offset + length == raw.size();
            zlib.push_back(last ? '\x01' : '\x00');
            zlib.push_back(static_cast<char>(length & 0xFF));
            zlib.push_back(static_cast<char>(length >> 8));
            zlib.push_back(static_cast<char>(~length & 0xFF));
            zlib.push_back(static_cast<char>((~length >> 8) & 0xFF));
            zlib.append(raw, offset, length);
            offset += length;
        } while (offset < raw.size());
        std::uint32_t a = 1;
        std::uint32_t b = 0;
        for (const char byte: raw) {
            a = (a + static_cast<std::uint8_t>(byte)) % 65521;
            b = (b + a) % 65521;
        }
        appendBigEndianU32(zlib, (b << 16) | a);

        std::string header;
        appendBigEndianU32(header, width);
        appendBigEndianU32(header, height);
        header += std::string("\x08\x06\x00\x00\x00", 5);

        std::string file = "\x89PNG\r\n\x1a\n";
        appendPngChunk(file, "IHDR", header);
        appendPngChunk(file, "IDAT", zlib);
        appendPngChunk(file, "IEND", "");
        return file;
    }

    inline std::vector<std::uint8_t> solidPixels(const std::uint32_t width, const std::uint32_t height, const std::array<std::uint8_t, 4> rgba) {
        std::vector<std::uint8_t> pixels;
        for (std::uint32_t i = 0; i < width * height; i++) {
            pixels.insert(pixels.end(), rgba.begin(), rgba.end());
        }
        return pixels;
    }

    inline std::string solidPng(const std::uint32_t width, const std::uint32_t height, const std::array<std::uint8_t, 4> rgba) {
        return pngFile(width, height, solidPixels(width, height, rgba));
    }

    // A legacy DDS header with the four-character code fourCc, or with an uncompressed 32-bit RGBA layout when fourCc is
    // empty, followed by data.
    inline std::string ddsFile(const std::uint32_t width, const std::uint32_t height, const std::string_view fourCc, const std::string& data) {
        std::string file = "DDS ";
        std::string header(124, '\0');
        const auto put = [&header](const std::size_t offset, const std::uint32_t value) {
            for (std::size_t i = 0; i < 4; i++) {
                header[offset + i] = static_cast<char>((value >> (8 * i)) & 0xFFu);
            }
        };
        put(0, 124);
        put(4, 0x1007);
        put(8, height);
        put(12, width);
        put(72, 32);
        if (fourCc.empty()) {
            put(76, 0x41);
            put(84, 32);
            put(88, 0x000000FFu);
            put(92, 0x0000FF00u);
            put(96, 0x00FF0000u);
            put(100, 0xFF000000u);
        } else {
            put(76, 0x4);
            header.replace(80, 4, fourCc.substr(0, 4));
        }
        file += header;
        file += data;
        return file;
    }

    // A BC1 block whose 16 pixels all take color0, a 5:6:5 colour.
    inline std::string solidBc1Block(const std::uint16_t color565) {
        std::string block;
        block.push_back(static_cast<char>(color565 & 0xFF));
        block.push_back(static_cast<char>(color565 >> 8));
        block.append(2, '\0');
        block.append(4, '\0');
        return block;
    }

    // Each level's texels, four bytes each, top row first. BC5 fills red and green and leaves blue 0 and alpha 255.
    inline std::vector<std::uint8_t> decodeLevel(const TechEngine::Texture& texture, const std::uint32_t level) {
        const std::uint32_t width = std::max(texture.width >> level, 1u);
        const std::uint32_t height = std::max(texture.height >> level, 1u);
        const std::uint32_t blocksAcross = (width + 3) / 4;
        const std::uint32_t blocksDown = (height + 3) / 4;
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 4, 255);
        const std::vector<std::byte>& data = texture.mips[level];
        for (std::uint32_t blockY = 0; blockY < blocksDown; blockY++) {
            for (std::uint32_t blockX = 0; blockX < blocksAcross; blockX++) {
                const std::byte* block = data.data() + (static_cast<std::size_t>(blockY) * blocksAcross + blockX) * 16;
                std::array<std::uint8_t, 64> decoded{};
                if (texture.format == TechEngine::TextureFormat::Bc5Unorm) {
                    std::array<std::uint8_t, 32> redGreen{};
                    bcdec_bc5(block, redGreen.data(), 8);
                    for (std::size_t i = 0; i < 16; i++) {
                        decoded[i * 4] = redGreen[i * 2];
                        decoded[i * 4 + 1] = redGreen[i * 2 + 1];
                        decoded[i * 4 + 3] = 255;
                    }
                } else {
                    bcdec_bc7(block, decoded.data(), 16);
                }
                for (std::uint32_t y = 0; y < 4; y++) {
                    for (std::uint32_t x = 0; x < 4; x++) {
                        const std::uint32_t imageX = blockX * 4 + x;
                        const std::uint32_t imageY = blockY * 4 + y;
                        if (imageX < width && imageY < height) {
                            std::copy_n(&decoded[(y * 4 + x) * 4], 4, &pixels[(static_cast<std::size_t>(imageY) * width + imageX) * 4]);
                        }
                    }
                }
            }
        }
        return pixels;
    }
}
