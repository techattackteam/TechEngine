#include <assets/ImageDecoder.hpp>
#include <assets/ImageTestFiles.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using TechEngine::ByteImage;
using TechEngine::FloatImage;
using TechEngine::ImageDecoder;
using TechEngine::ProjectError;

static std::span<const std::byte> asBytes(const std::string_view text) {
    return std::as_bytes(std::span<const char>{text.data(), text.size()});
}

TEST_CASE("a Radiance file decodes to linear floats with an alpha of 1", "[editor][assets][decoder]") {
    const std::string file = TechEngineTests::radianceFile(2, 1, {TechEngineTests::RgbePixel{128, 64, 32, 129}, TechEngineTests::RgbePixel{0, 0, 0, 0}});

    FloatImage image;
    REQUIRE(ImageDecoder::decodeFloat(asBytes(file), image) == std::error_code{});

    CHECK(image.width == 2);
    CHECK(image.height == 1);
    REQUIRE(image.pixels.size() == 8);
    CHECK(image.pixels[0] == TechEngineTests::rgbeChannel(128, 129));
    CHECK(image.pixels[1] == TechEngineTests::rgbeChannel(64, 129));
    CHECK(image.pixels[2] == TechEngineTests::rgbeChannel(32, 129));
    CHECK(image.pixels[0] == Catch::Approx(1.0f).margin(0.01f));
    CHECK(image.pixels[3] == 1.0f);
    CHECK(image.pixels[4] == 0.0f);
    CHECK(image.pixels[7] == 1.0f);
}

TEST_CASE("an uncompressed TGA decodes to 8-bit RGBA, top row first", "[editor][assets][decoder]") {
    const std::string file = TechEngineTests::tgaFile(2, 1, {10, 20, 30, 40, 50, 60});

    ByteImage image;
    REQUIRE(ImageDecoder::decodeBytes(asBytes(file), image) == std::error_code{});

    CHECK(image.width == 2);
    CHECK(image.height == 1);
    const std::vector<std::uint8_t> expected{30, 20, 10, 255, 60, 50, 40, 255};
    CHECK(image.pixels == expected);
}

TEST_CASE("bytes that are not an image are refused and leave the output as it was", "[editor][assets][decoder]") {
    FloatImage floatImage;
    floatImage.width = 7;
    ByteImage byteImage;
    byteImage.width = 7;
    const std::string garbage = "this is not an image at all";

    CHECK(ImageDecoder::decodeFloat(asBytes(garbage), floatImage) == ProjectError::ImageUnreadable);
    CHECK(ImageDecoder::decodeBytes(asBytes(garbage), byteImage) == ProjectError::ImageUnreadable);
    CHECK(ImageDecoder::decodeFloat({}, floatImage) == ProjectError::ImageUnreadable);
    CHECK(ImageDecoder::decodeBytes({}, byteImage) == ProjectError::ImageUnreadable);
    CHECK(floatImage.width == 7);
    CHECK(byteImage.width == 7);
}

TEST_CASE("a file cut inside its header is refused", "[editor][assets][decoder]") {
    const std::string whole = TechEngineTests::gradientRadianceFile(5, 3);
    const std::string cut = whole.substr(0, 20);

    FloatImage image;
    CHECK(ImageDecoder::decodeFloat(asBytes(cut), image) == ProjectError::ImageUnreadable);
}

TEST_CASE("a side longer than the editor's limit is refused by both decoders, and one at the limit decodes", "[editor][assets][decoder]") {
    const std::uint32_t limit = ImageDecoder::MAX_IMAGE_SIDE;
    const TechEngineTests::RgbePixel grey{128, 128, 128, 128};
    FloatImage floatImage;
    ByteImage byteImage;

    SECTION("too wide") {
        const std::string file = TechEngineTests::radianceFile(limit + 1, 1, std::vector<TechEngineTests::RgbePixel>(limit + 1, grey));
        CHECK(ImageDecoder::decodeFloat(asBytes(file), floatImage) == ProjectError::ImageUnreadable);
        CHECK(ImageDecoder::decodeBytes(asBytes(file), byteImage) == ProjectError::ImageUnreadable);
        const std::string tga = TechEngineTests::tgaFile(static_cast<std::uint16_t>(limit + 1), 1, std::vector<std::uint8_t>(static_cast<std::size_t>(limit + 1) * 3, 100));
        CHECK(ImageDecoder::decodeBytes(asBytes(tga), byteImage) == ProjectError::ImageUnreadable);
        CHECK(floatImage.width == 0);
        CHECK(byteImage.width == 0);
    }

    SECTION("too tall") {
        const std::string file = TechEngineTests::radianceFile(1, limit + 1, std::vector<TechEngineTests::RgbePixel>(limit + 1, grey));
        CHECK(ImageDecoder::decodeFloat(asBytes(file), floatImage) == ProjectError::ImageUnreadable);
        CHECK(ImageDecoder::decodeBytes(asBytes(file), byteImage) == ProjectError::ImageUnreadable);
    }

    SECTION("at the limit") {
        const std::string file = TechEngineTests::radianceFile(limit, 1, std::vector<TechEngineTests::RgbePixel>(limit, grey));
        REQUIRE(ImageDecoder::decodeFloat(asBytes(file), floatImage) == std::error_code{});
        CHECK(floatImage.width == limit);
        CHECK(floatImage.pixels.size() == static_cast<std::size_t>(limit) * 4);
        const std::string tga = TechEngineTests::tgaFile(1, static_cast<std::uint16_t>(limit), std::vector<std::uint8_t>(static_cast<std::size_t>(limit) * 3, 100));
        REQUIRE(ImageDecoder::decodeBytes(asBytes(tga), byteImage) == std::error_code{});
        CHECK(byteImage.height == limit);
    }
}
