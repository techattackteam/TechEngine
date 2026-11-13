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
