#include <assets/DdsReader.hpp>
#include <assets/ImageDecoder.hpp>
#include <assets/TextureTestFiles.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::ByteImage;
using TechEngine::DdsReader;
using TechEngine::ProjectError;

static std::span<const std::byte> asBytes(const std::string_view text) {
    return std::as_bytes(std::span<const char>{text.data(), text.size()});
}

static std::vector<std::uint8_t> pixelAt(const ByteImage& image, const std::uint32_t x, const std::uint32_t y) {
    const std::size_t offset = (static_cast<std::size_t>(y) * image.width + x) * 4;
    return {image.pixels.begin() + static_cast<std::ptrdiff_t>(offset), image.pixels.begin() + static_cast<std::ptrdiff_t>(offset + 4)};
}

TEST_CASE("a BC1 DDS file decodes to its blocks' colours, cut to the image's size", "[editor][assets][dds]") {
    // 6x5 takes 2x2 blocks: red, green, blue, white, row by row.
    const std::string data = TechEngineTests::solidBc1Block(0xF800) + TechEngineTests::solidBc1Block(0x07E0) + TechEngineTests::solidBc1Block(0x001F) + TechEngineTests::solidBc1Block(0xFFFF);
    const std::string file = TechEngineTests::ddsFile(6, 5, "DXT1", data);
    REQUIRE(DdsReader::isDds(asBytes(file)));

    ByteImage image;
    REQUIRE(DdsReader::decode(asBytes(file), image) == std::error_code{});

    CHECK(image.width == 6);
    CHECK(image.height == 5);
    REQUIRE(image.pixels.size() == 6 * 5 * 4);
    CHECK(pixelAt(image, 0, 0) == std::vector<std::uint8_t>{255, 0, 0, 255});
    CHECK(pixelAt(image, 3, 3) == std::vector<std::uint8_t>{255, 0, 0, 255});
    CHECK(pixelAt(image, 4, 0) == std::vector<std::uint8_t>{0, 255, 0, 255});
    CHECK(pixelAt(image, 0, 4) == std::vector<std::uint8_t>{0, 0, 255, 255});
    CHECK(pixelAt(image, 5, 4) == std::vector<std::uint8_t>{255, 255, 255, 255});
}

TEST_CASE("an uncompressed 32-bit DDS file keeps its bytes", "[editor][assets][dds]") {
    const std::string data = {'\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08'};
    const std::string file = TechEngineTests::ddsFile(2, 1, "", data);

    ByteImage image;
    REQUIRE(DdsReader::decode(asBytes(file), image) == std::error_code{});

    CHECK(image.width == 2);
    CHECK(image.height == 1);
    CHECK(image.pixels == std::vector<std::uint8_t>{1, 2, 3, 4, 5, 6, 7, 8});
}

TEST_CASE("a BC5 DDS file rebuilds blue as the normal's z", "[editor][assets][dds]") {
    // Both channels' endpoints are 128, so every texel is (128, 128): a normal close to +Z.
    const std::string channel = std::string("\x80\x80", 2) + std::string(6, '\0');
    const std::string file = TechEngineTests::ddsFile(4, 4, "ATI2", channel + channel);

    ByteImage image;
    REQUIRE(DdsReader::decode(asBytes(file), image) == std::error_code{});

    const std::vector<std::uint8_t> texel = pixelAt(image, 2, 1);
    CHECK(texel[0] == 128);
    CHECK(texel[1] == 128);
    CHECK(texel[2] == 255);
    CHECK(texel[3] == 255);
}

TEST_CASE("a DDS file in a format the reader does not decode is refused", "[editor][assets][dds]") {
    SECTION("a legacy four-character code") {
        const std::string file = TechEngineTests::ddsFile(4, 4, "DXT2", std::string(16, '\0'));
        ByteImage image;
        CHECK(DdsReader::decode(asBytes(file), image) == ProjectError::UnsupportedDdsFormat);
    }

    SECTION("a DX10 header naming BC6H") {
        std::string file = TechEngineTests::ddsFile(4, 4, "DX10", "");
        TechEngineTests::appendLittleEndianU32(file, 95);
        file += std::string(16 + 16, '\0');
        ByteImage image;
        CHECK(DdsReader::decode(asBytes(file), image) == ProjectError::UnsupportedDdsFormat);
    }
}

TEST_CASE("a malformed DDS file is unreadable", "[editor][assets][dds]") {
    ByteImage image;

    SECTION("blocks cut short") {
        const std::string file = TechEngineTests::ddsFile(8, 8, "DXT1", std::string(8 * 3, '\0'));
        CHECK(DdsReader::decode(asBytes(file), image) == ProjectError::ImageUnreadable);
    }

    SECTION("a header cut short") {
        const std::string file = TechEngineTests::ddsFile(4, 4, "DXT1", "").substr(0, 60);
        CHECK(DdsReader::decode(asBytes(file), image) == ProjectError::ImageUnreadable);
    }

    SECTION("a side of zero") {
        const std::string file = TechEngineTests::ddsFile(0, 4, "DXT1", std::string(8, '\0'));
        CHECK(DdsReader::decode(asBytes(file), image) == ProjectError::ImageUnreadable);
    }

    SECTION("a file that is not a DDS") {
        CHECK_FALSE(DdsReader::isDds(asBytes("PNG and more")));
        CHECK(DdsReader::decode(asBytes("PNG and more"), image) == ProjectError::ImageUnreadable);
    }
}
