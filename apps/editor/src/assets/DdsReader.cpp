#include <TechEngine/base/diagnostics/Log.hpp>

#include <assets/DdsReader.hpp>
#include <project/ProjectError.hpp>

#include <bcdec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <system_error>

namespace TechEngine {
    static constexpr std::string_view DDS_MAGIC = "DDS ";
    static constexpr std::uint32_t DDS_HEADER_SIZE = 124;
    static constexpr std::size_t HEADER_SIZE_OFFSET = 4;
    static constexpr std::size_t HEIGHT_OFFSET = 12;
    static constexpr std::size_t WIDTH_OFFSET = 16;
    static constexpr std::size_t PIXEL_FLAGS_OFFSET = 80;
    static constexpr std::size_t FOUR_CC_OFFSET = 84;
    static constexpr std::size_t BIT_COUNT_OFFSET = 88;
    static constexpr std::size_t RED_MASK_OFFSET = 92;
    static constexpr std::size_t GREEN_MASK_OFFSET = 96;
    static constexpr std::size_t BLUE_MASK_OFFSET = 100;
    static constexpr std::size_t ALPHA_MASK_OFFSET = 104;
    static constexpr std::size_t LEGACY_DATA_OFFSET = 128;
    static constexpr std::size_t DX10_FORMAT_OFFSET = 128;
    static constexpr std::size_t DX10_DATA_OFFSET = 148;
    static constexpr std::uint32_t PIXEL_FLAG_ALPHA = 0x1;
    static constexpr std::uint32_t PIXEL_FLAG_FOUR_CC = 0x4;
    static constexpr std::uint32_t PIXEL_FLAG_RGB = 0x40;
    // Four ASCII characters read as a little-endian u32.
    static constexpr std::uint32_t FOUR_CC_DXT1 = 0x31545844;
    static constexpr std::uint32_t FOUR_CC_DXT3 = 0x33545844;
    static constexpr std::uint32_t FOUR_CC_DXT5 = 0x35545844;
    static constexpr std::uint32_t FOUR_CC_ATI2 = 0x32495441;
    static constexpr std::uint32_t FOUR_CC_BC5U = 0x55354342;
    static constexpr std::uint32_t FOUR_CC_DX10 = 0x30315844;
    static constexpr std::uint32_t DXGI_R8G8B8A8_UNORM = 28;
    static constexpr std::uint32_t DXGI_R8G8B8A8_UNORM_SRGB = 29;
    static constexpr std::uint32_t DXGI_BC1_UNORM = 71;
    static constexpr std::uint32_t DXGI_BC1_UNORM_SRGB = 72;
    static constexpr std::uint32_t DXGI_BC2_UNORM = 74;
    static constexpr std::uint32_t DXGI_BC2_UNORM_SRGB = 75;
    static constexpr std::uint32_t DXGI_BC3_UNORM = 77;
    static constexpr std::uint32_t DXGI_BC3_UNORM_SRGB = 78;
    static constexpr std::uint32_t DXGI_BC5_UNORM = 83;
    static constexpr std::uint32_t DXGI_B8G8R8A8_UNORM = 87;
    static constexpr std::uint32_t DXGI_B8G8R8A8_UNORM_SRGB = 91;
    static constexpr std::uint32_t DXGI_BC7_UNORM = 98;
    static constexpr std::uint32_t DXGI_BC7_UNORM_SRGB = 99;
    static constexpr std::size_t CHANNELS = 4;
    static constexpr std::size_t BLOCK_SIDE = 4;
    static constexpr std::size_t BLOCK_PIXELS = BLOCK_SIDE * BLOCK_SIDE;

    bool DdsReader::isDds(const std::span<const std::byte> bytes) {
        const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        return text.starts_with(DDS_MAGIC);
    }

    std::error_code DdsReader::decode(const std::span<const std::byte> bytes, ByteImage& out) {
        Layout layout = Layout::Rgba8;
        std::size_t dataOffset = 0;
        const std::error_code layoutError = readLayout(bytes, layout, dataOffset);
        if (layoutError) {
            return layoutError;
        }

        const std::uint32_t width = readU32(bytes, WIDTH_OFFSET);
        const std::uint32_t height = readU32(bytes, HEIGHT_OFFSET);
        if (width == 0 || height == 0 || width > ImageDecoder::MAX_IMAGE_SIDE || height > ImageDecoder::MAX_IMAGE_SIDE) {
            return ProjectError::ImageUnreadable;
        }
        const std::uint64_t size = topLevelSize(layout, width, height);
        if (size > bytes.size() - dataOffset) {
            return ProjectError::ImageUnreadable;
        }

        out.width = width;
        out.height = height;
        out.pixels.assign(static_cast<std::size_t>(width) * height * CHANNELS, 0);
        const std::span<const std::byte> data = bytes.subspan(dataOffset, static_cast<std::size_t>(size));
        if (layout == Layout::Rgba8 || layout == Layout::Bgra8 || layout == Layout::Rgbx8 || layout == Layout::Bgrx8) {
            decodePixels(data, layout, out);
        } else {
            decodeBlocks(data, layout, out);
        }
        return {};
    }

    std::error_code DdsReader::readLayout(const std::span<const std::byte> bytes, Layout& layout, std::size_t& dataOffset) {
        if (!isDds(bytes) || bytes.size() < LEGACY_DATA_OFFSET || readU32(bytes, HEADER_SIZE_OFFSET) != DDS_HEADER_SIZE) {
            return ProjectError::ImageUnreadable;
        }

        const std::uint32_t pixelFlags = readU32(bytes, PIXEL_FLAGS_OFFSET);
        if ((pixelFlags & PIXEL_FLAG_FOUR_CC) == 0) {
            dataOffset = LEGACY_DATA_OFFSET;
            if ((pixelFlags & PIXEL_FLAG_RGB) != 0 && layoutFromMasks(bytes, layout)) {
                return {};
            }
            TE_LOGGER_WARN("Import: a DDS file's uncompressed format, {0} bits with flags {1:#x}, is not decoded", readU32(bytes, BIT_COUNT_OFFSET), pixelFlags);
            return ProjectError::UnsupportedDdsFormat;
        }

        const std::uint32_t fourCc = readU32(bytes, FOUR_CC_OFFSET);
        dataOffset = LEGACY_DATA_OFFSET;
        switch (fourCc) {
            case FOUR_CC_DXT1:
                layout = Layout::Bc1;
                return {};
            case FOUR_CC_DXT3:
                layout = Layout::Bc2;
                return {};
            case FOUR_CC_DXT5:
                layout = Layout::Bc3;
                return {};
            case FOUR_CC_ATI2:
            case FOUR_CC_BC5U:
                layout = Layout::Bc5;
                return {};
            case FOUR_CC_DX10:
                break;
            default:
                TE_LOGGER_WARN("Import: a DDS file's four-character code {0:#010x} is not decoded", fourCc);
                return ProjectError::UnsupportedDdsFormat;
        }

        if (bytes.size() < DX10_DATA_OFFSET) {
            return ProjectError::ImageUnreadable;
        }
        dataOffset = DX10_DATA_OFFSET;
        const std::uint32_t dxgiFormat = readU32(bytes, DX10_FORMAT_OFFSET);
        if (!layoutFromDxgi(dxgiFormat, layout)) {
            TE_LOGGER_WARN("Import: a DDS file's DXGI format {0} is not decoded", dxgiFormat);
            return ProjectError::UnsupportedDdsFormat;
        }
        return {};
    }

    bool DdsReader::layoutFromDxgi(const std::uint32_t dxgiFormat, Layout& out) {
        switch (dxgiFormat) {
            case DXGI_BC1_UNORM:
            case DXGI_BC1_UNORM_SRGB:
                out = Layout::Bc1;
                return true;
            case DXGI_BC2_UNORM:
            case DXGI_BC2_UNORM_SRGB:
                out = Layout::Bc2;
                return true;
            case DXGI_BC3_UNORM:
            case DXGI_BC3_UNORM_SRGB:
                out = Layout::Bc3;
                return true;
            case DXGI_BC5_UNORM:
                out = Layout::Bc5;
                return true;
            case DXGI_BC7_UNORM:
            case DXGI_BC7_UNORM_SRGB:
                out = Layout::Bc7;
                return true;
            case DXGI_R8G8B8A8_UNORM:
            case DXGI_R8G8B8A8_UNORM_SRGB:
                out = Layout::Rgba8;
                return true;
            case DXGI_B8G8R8A8_UNORM:
            case DXGI_B8G8R8A8_UNORM_SRGB:
                out = Layout::Bgra8;
                return true;
            default:
                return false;
        }
    }

    bool DdsReader::layoutFromMasks(const std::span<const std::byte> bytes, Layout& out) {
        if (readU32(bytes, BIT_COUNT_OFFSET) != 32) {
            return false;
        }
        const std::uint32_t redMask = readU32(bytes, RED_MASK_OFFSET);
        const std::uint32_t greenMask = readU32(bytes, GREEN_MASK_OFFSET);
        const std::uint32_t blueMask = readU32(bytes, BLUE_MASK_OFFSET);
        const bool hasAlpha = (readU32(bytes, PIXEL_FLAGS_OFFSET) & PIXEL_FLAG_ALPHA) != 0 && readU32(bytes, ALPHA_MASK_OFFSET) == 0xFF000000u;
        if (greenMask != 0x0000FF00u) {
            return false;
        }
        if (redMask == 0x000000FFu && blueMask == 0x00FF0000u) {
            out = hasAlpha ? Layout::Rgba8 : Layout::Rgbx8;
            return true;
        }
        if (redMask == 0x00FF0000u && blueMask == 0x000000FFu) {
            out = hasAlpha ? Layout::Bgra8 : Layout::Bgrx8;
            return true;
        }
        return false;
    }

    std::uint64_t DdsReader::topLevelSize(const Layout layout, const std::uint32_t width, const std::uint32_t height) {
        const std::uint64_t blocks = ((static_cast<std::uint64_t>(width) + 3) / 4) * ((static_cast<std::uint64_t>(height) + 3) / 4);
        switch (layout) {
            case Layout::Bc1:
                return blocks * BCDEC_BC1_BLOCK_SIZE;
            case Layout::Bc2:
            case Layout::Bc3:
            case Layout::Bc5:
            case Layout::Bc7:
                return blocks * BCDEC_BC7_BLOCK_SIZE;
            case Layout::Rgba8:
            case Layout::Bgra8:
            case Layout::Rgbx8:
            case Layout::Bgrx8:
                break;
        }
        return static_cast<std::uint64_t>(width) * height * CHANNELS;
    }

    // A block past the right or bottom edge decodes in full, and only the pixels inside the image are kept.
    void DdsReader::decodeBlocks(const std::span<const std::byte> data, const Layout layout, ByteImage& out) {
        const std::size_t blockSize = layout == Layout::Bc1 ? BCDEC_BC1_BLOCK_SIZE : BCDEC_BC7_BLOCK_SIZE;
        const std::uint32_t blocksAcross = (out.width + 3) / 4;
        const std::uint32_t blocksDown = (out.height + 3) / 4;
        std::array<std::uint8_t, BLOCK_PIXELS * CHANNELS> block{};
        for (std::uint32_t blockY = 0; blockY < blocksDown; blockY++) {
            for (std::uint32_t blockX = 0; blockX < blocksAcross; blockX++) {
                const std::byte* source = data.data() + (static_cast<std::size_t>(blockY) * blocksAcross + blockX) * blockSize;
                switch (layout) {
                    case Layout::Bc1:
                        bcdec_bc1(source, block.data(), BLOCK_SIDE * CHANNELS);
                        break;
                    case Layout::Bc2:
                        bcdec_bc2(source, block.data(), BLOCK_SIDE * CHANNELS);
                        break;
                    case Layout::Bc3:
                        bcdec_bc3(source, block.data(), BLOCK_SIDE * CHANNELS);
                        break;
                    case Layout::Bc5: {
                        std::array<std::uint8_t, BLOCK_PIXELS * 2> redGreen{};
                        bcdec_bc5(source, redGreen.data(), BLOCK_SIDE * 2);
                        for (std::size_t pixel = 0; pixel < BLOCK_PIXELS; pixel++) {
                            block[pixel * CHANNELS] = redGreen[pixel * 2];
                            block[pixel * CHANNELS + 1] = redGreen[pixel * 2 + 1];
                            block[pixel * CHANNELS + 2] = rebuildNormalZ(redGreen[pixel * 2], redGreen[pixel * 2 + 1]);
                            block[pixel * CHANNELS + 3] = 255;
                        }
                        break;
                    }
                    case Layout::Bc7:
                        bcdec_bc7(source, block.data(), BLOCK_SIDE * CHANNELS);
                        break;
                    case Layout::Rgba8:
                    case Layout::Bgra8:
                    case Layout::Rgbx8:
                    case Layout::Bgrx8:
                        return;
                }

                for (std::uint32_t y = 0; y < BLOCK_SIDE; y++) {
                    const std::uint32_t imageY = blockY * BLOCK_SIDE + y;
                    for (std::uint32_t x = 0; x < BLOCK_SIDE; x++) {
                        const std::uint32_t imageX = blockX * BLOCK_SIDE + x;
                        if (imageX >= out.width || imageY >= out.height) {
                            continue;
                        }
                        const std::uint8_t* texel = &block[(y * BLOCK_SIDE + x) * CHANNELS];
                        std::copy_n(texel, CHANNELS, &out.pixels[(static_cast<std::size_t>(imageY) * out.width + imageX) * CHANNELS]);
                    }
                }
            }
        }
    }

    void DdsReader::decodePixels(const std::span<const std::byte> data, const Layout layout, ByteImage& out) {
        const bool swapRedBlue = layout == Layout::Bgra8 || layout == Layout::Bgrx8;
        const bool opaque = layout == Layout::Rgbx8 || layout == Layout::Bgrx8;
        for (std::size_t pixel = 0; pixel < static_cast<std::size_t>(out.width) * out.height; pixel++) {
            const std::byte* source = &data[pixel * CHANNELS];
            std::uint8_t* destination = &out.pixels[pixel * CHANNELS];
            destination[0] = static_cast<std::uint8_t>(source[swapRedBlue ? 2 : 0]);
            destination[1] = static_cast<std::uint8_t>(source[1]);
            destination[2] = static_cast<std::uint8_t>(source[swapRedBlue ? 0 : 2]);
            destination[3] = opaque ? 255 : static_cast<std::uint8_t>(source[3]);
        }
    }

    std::uint8_t DdsReader::rebuildNormalZ(const std::uint8_t x, const std::uint8_t y) {
        const float normalX = static_cast<float>(x) / 127.5f - 1.0f;
        const float normalY = static_cast<float>(y) / 127.5f - 1.0f;
        const float normalZ = std::sqrt(std::max(0.0f, 1.0f - normalX * normalX - normalY * normalY));
        return static_cast<std::uint8_t>(std::lround((normalZ * 0.5f + 0.5f) * 255.0f));
    }

    std::uint32_t DdsReader::readU32(const std::span<const std::byte> bytes, const std::size_t offset) {
        std::uint32_t value = 0;
        for (std::size_t i = 0; i < 4; i++) {
            value |= static_cast<std::uint32_t>(bytes[offset + i]) << (8 * i);
        }
        return value;
    }
}
