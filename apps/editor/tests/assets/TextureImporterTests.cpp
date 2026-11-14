#include <TechEngine/core/texture/Texture.hpp>

#include <assets/ImageDecoder.hpp>
#include <assets/TextureImporter.hpp>
#include <assets/TextureTestFiles.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <utility>
#include <vector>

using TechEngine::ByteImage;
using TechEngine::FloatImage;
using TechEngine::Texture;
using TechEngine::TextureFilter;
using TechEngine::TextureFormat;
using TechEngine::TextureImporter;
using TechEngine::TexturePurpose;
using TechEngine::TextureSampler;
using TechEngine::TextureWrap;

static constexpr int BC_TOLERANCE = 4;

static ByteImage makeImage(const std::uint32_t width, const std::uint32_t height, const std::vector<std::uint8_t>& pixels) {
    ByteImage image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    return image;
}

static bool near(const int actual, const int expected) {
    return std::abs(actual - expected) <= BC_TOLERANCE;
}

static FloatImage makeFloatImage(const std::uint32_t width, const std::uint32_t height, const float firstValue) {
    FloatImage image;
    image.width = width;
    image.height = height;
    for (std::uint32_t i = 0; i < width * height * 4; i++) {
        image.pixels.push_back(firstValue + static_cast<float>(i));
    }
    return image;
}

static float channel(const FloatImage& image, const std::uint32_t x, const std::uint32_t y, const std::uint32_t index) {
    return image.pixels[(static_cast<std::size_t>(y) * image.width + x) * 4 + index];
}

static constexpr std::uint32_t EXPONENT_SHIFT = 27;

static void unpackRed(const std::uint32_t packed, float& red) {
    float green = 0.0f;
    float blue = 0.0f;
    TextureImporter::unpackRgb9E5(packed, red, green, blue);
}

TEST_CASE("each purpose has its format and a full chain of correctly sized levels", "[editor][assets][textures]") {
    const ByteImage image = makeImage(13, 6, TechEngineTests::solidPixels(13, 6, {10, 20, 30, 255}));
    const std::array<std::pair<TexturePurpose, TextureFormat>, 4> purposes{{
        {TexturePurpose::BaseColor, TextureFormat::Bc7Srgb},
        {TexturePurpose::OcclusionRoughnessMetallic, TextureFormat::Bc7Unorm},
        {TexturePurpose::Normal, TextureFormat::Bc5Unorm},
        {TexturePurpose::Emissive, TextureFormat::Bc7Srgb},
    }};

    for (const auto& [purpose, format]: purposes) {
        CHECK(TextureImporter::formatOf(purpose) == format);
        Texture texture;
        TextureImporter::encode(image, purpose, texture);
        CHECK(texture.format == format);
        CHECK(texture.width == 13);
        CHECK(texture.height == 6);
        REQUIRE(texture.mips.size() == Texture::mipCount(13, 6));
        for (std::uint32_t level = 0; level < texture.mips.size(); level++) {
            CHECK(texture.mips[level].size() == Texture::mipByteSize(format, 13, 6, level));
        }
    }
}

TEST_CASE("base color mips are filtered in linear light and stored as sRGB, and ORM mips stay linear", "[editor][assets][textures]") {
    // A black and white checker: its linear mean is 0.5, which sRGB stores as 188, not 128.
    std::vector<std::uint8_t> pixels = {0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 0, 255};
    const ByteImage checker = makeImage(2, 2, pixels);

    Texture baseColor;
    TextureImporter::encode(checker, TexturePurpose::BaseColor, baseColor);
    REQUIRE(baseColor.mips.size() == 2);
    const std::vector<std::uint8_t> topLevel = TechEngineTests::decodeLevel(baseColor, 0);
    CHECK(near(topLevel[0], 0));
    CHECK(near(topLevel[4], 255));
    const std::vector<std::uint8_t> srgbMean = TechEngineTests::decodeLevel(baseColor, 1);
    CHECK(near(srgbMean[0], 188));
    CHECK(near(srgbMean[1], 188));
    CHECK(near(srgbMean[2], 188));
    CHECK(near(srgbMean[3], 255));

    Texture packed;
    TextureImporter::encode(checker, TexturePurpose::OcclusionRoughnessMetallic, packed);
    const std::vector<std::uint8_t> linearMean = TechEngineTests::decodeLevel(packed, 1);
    CHECK(near(linearMean[0], 128));
    CHECK(near(linearMean[1], 128));
    CHECK(near(linearMean[2], 128));
}

TEST_CASE("a normal map's mips keep unit normals in red and green", "[editor][assets][textures]") {
    // Two normals leaning opposite ways along X: their mean is +Z after renormalizing, not a short vector.
    const std::vector<std::uint8_t> pixels = {218, 128, 218, 255, 37, 128, 218, 255, 218, 128, 218, 255, 37, 128, 218, 255};
    Texture texture;
    TextureImporter::encode(makeImage(2, 2, pixels), TexturePurpose::Normal, texture);

    REQUIRE(texture.mips.size() == 2);
    const std::vector<std::uint8_t> mean = TechEngineTests::decodeLevel(texture, 1);
    CHECK(near(mean[0], 128));
    CHECK(near(mean[1], 128));
}

TEST_CASE("an image smaller than a block fills a whole block", "[editor][assets][textures]") {
    Texture texture;
    TextureImporter::encode(makeImage(1, 1, {40, 80, 120, 200}), TexturePurpose::BaseColor, texture);

    REQUIRE(texture.mips.size() == 1);
    CHECK(texture.mips[0].size() == 16);
    const std::vector<std::uint8_t> texel = TechEngineTests::decodeLevel(texture, 0);
    CHECK(near(texel[0], 40));
    CHECK(near(texel[1], 80));
    CHECK(near(texel[2], 120));
    CHECK(near(texel[3], 200));
}

TEST_CASE("packing takes red from occlusion and green and blue from metallic-roughness, at the larger size", "[editor][assets][textures]") {
    const ByteImage occlusion = makeImage(1, 1, {200, 1, 2, 3});
    const ByteImage metallicRoughness = makeImage(2, 2, TechEngineTests::solidPixels(2, 2, {4, 100, 50, 6}));
    const ByteImage missing;
    ByteImage packed;

    SECTION("both images") {
        TextureImporter::packOcclusionRoughnessMetallic(occlusion, metallicRoughness, packed);
        CHECK(packed.width == 2);
        CHECK(packed.height == 2);
        CHECK(packed.pixels == TechEngineTests::solidPixels(2, 2, {200, 100, 50, 255}));
    }

    SECTION("no occlusion gives a red of 1") {
        TextureImporter::packOcclusionRoughnessMetallic(missing, metallicRoughness, packed);
        CHECK(packed.pixels == TechEngineTests::solidPixels(2, 2, {255, 100, 50, 255}));
    }

    SECTION("no metallic-roughness gives a green and a blue of 1") {
        TextureImporter::packOcclusionRoughnessMetallic(occlusion, missing, packed);
        CHECK(packed.width == 1);
        CHECK(packed.pixels == std::vector<std::uint8_t>{200, 255, 255, 255});
    }

    SECTION("a wider image and a taller one meet at the larger of each side, scaled bilinearly") {
        const ByteImage ramp = makeImage(2, 1, {0, 0, 0, 0, 255, 0, 0, 0});
        TextureImporter::packOcclusionRoughnessMetallic(ramp, makeImage(1, 3, TechEngineTests::solidPixels(1, 3, {0, 9, 9, 0})), packed);
        CHECK(packed.width == 2);
        CHECK(packed.height == 3);
        CHECK(packed.pixels[0] == 0);
        CHECK(packed.pixels[4] == 255);
        CHECK(packed.pixels[1] == 9);

        ByteImage stretched;
        TextureImporter::packOcclusionRoughnessMetallic(ramp, makeImage(4, 1, TechEngineTests::solidPixels(4, 1, {0, 0, 0, 0})), stretched);
        REQUIRE(stretched.width == 4);
        CHECK(stretched.pixels[0] == 0);
        CHECK(stretched.pixels[4] > 0);
        CHECK(stretched.pixels[4] < stretched.pixels[8]);
        CHECK(stretched.pixels[12] == 255);
    }
}

TEST_CASE("a texture's key name holds the image, the purpose and the sampler", "[editor][assets][textures]") {
    CHECK(TextureImporter::keyName("Textures/wall.png", TexturePurpose::Normal, TextureSampler{}) == "Textures/wall.png#normal#repeat-linear");
    CHECK(TextureImporter::keyName("ao.png+mr.png", TexturePurpose::OcclusionRoughnessMetallic, TextureSampler{}) == "ao.png+mr.png#orm#repeat-linear");
    CHECK(TextureImporter::keyName("*0", TexturePurpose::BaseColor, TextureSampler{TextureWrap::Clamp, TextureWrap::Clamp, TextureFilter::Nearest}) == "*0#basecolor#clamp-nearest");
    CHECK(TextureImporter::keyName("glow.png", TexturePurpose::Emissive, TextureSampler{TextureWrap::Mirror, TextureWrap::Repeat, TextureFilter::Linear}) == "glow.png#emissive#mirror-repeat-linear");
}

TEST_CASE("a 2x2 image averages to its mean in every channel", "[editor][assets][mips]") {
    FloatImage source;
    source.width = 2;
    source.height = 2;
    source.pixels = {1.0f, 10.0f, 100.0f, 1.0f, 3.0f, 20.0f, 200.0f, 0.0f, 5.0f, 30.0f, 300.0f, 1.0f, 7.0f, 40.0f, 400.0f, 0.0f};

    std::vector<FloatImage> chain;
    TextureImporter::buildMipChain(source, chain);

    REQUIRE(chain.size() == 1);
    CHECK(chain[0].width == 1);
    CHECK(chain[0].height == 1);
    REQUIRE(chain[0].pixels.size() == 4);
    CHECK(chain[0].pixels[0] == Catch::Approx(4.0f));
    CHECK(chain[0].pixels[1] == Catch::Approx(25.0f));
    CHECK(chain[0].pixels[2] == Catch::Approx(250.0f));
    CHECK(chain[0].pixels[3] == Catch::Approx(0.5f));
}

TEST_CASE("the chain has every level down to 1x1 and its length matches the texture's mip count", "[editor][assets][mips]") {
    const std::uint32_t sizes[][2] = {{1, 1}, {2, 1}, {1, 8}, {8, 4}, {5, 3}, {7, 7}, {16, 1}, {100, 37}};
    for (const auto& size: sizes) {
        const FloatImage source = makeFloatImage(size[0], size[1], 0.0f);
        std::vector<FloatImage> chain;
        TextureImporter::buildMipChain(source, chain);

        REQUIRE(chain.size() + 1 == Texture::mipCount(size[0], size[1]));
        for (std::size_t i = 0; i < chain.size(); i++) {
            const std::uint32_t level = static_cast<std::uint32_t>(i) + 1;
            CHECK(chain[i].width == std::max(size[0] >> level, 1u));
            CHECK(chain[i].height == std::max(size[1] >> level, 1u));
            CHECK(chain[i].pixels.size() == static_cast<std::size_t>(chain[i].width) * chain[i].height * 4);
        }
        CHECK(chain.empty() == (size[0] == 1 && size[1] == 1));
        if (!chain.empty()) {
            CHECK(chain.back().width == 1);
            CHECK(chain.back().height == 1);
        }
    }
}

TEST_CASE("an odd size halves down and leaves its last row and column out of the level", "[editor][assets][mips]") {
    const FloatImage source = makeFloatImage(3, 3, 0.0f);

    std::vector<FloatImage> chain;
    TextureImporter::buildMipChain(source, chain);

    REQUIRE(chain.size() == 1);
    CHECK(chain[0].width == 1);
    CHECK(chain[0].height == 1);
    for (std::uint32_t index = 0; index < 4; index++) {
        const float expected = (channel(source, 0, 0, index) + channel(source, 1, 0, index) + channel(source, 0, 1, index) + channel(source, 1, 1, index)) / 4.0f;
        CHECK(chain[0].pixels[index] == Catch::Approx(expected));
    }
}

TEST_CASE("each pixel of a wider level averages the 2x2 block at twice its position", "[editor][assets][mips]") {
    const FloatImage source = makeFloatImage(5, 3, 1.0f);

    FloatImage level;
    TextureImporter::downsample(source, level);

    REQUIRE(level.width == 2);
    REQUIRE(level.height == 1);
    for (std::uint32_t x = 0; x < 2; x++) {
        for (std::uint32_t index = 0; index < 4; index++) {
            const float expected = (channel(source, x * 2, 0, index) + channel(source, x * 2 + 1, 0, index) + channel(source, x * 2, 1, index) + channel(source, x * 2 + 1, 1, index)) / 4.0f;
            CHECK(channel(level, x, 0, index) == Catch::Approx(expected));
        }
    }
}

TEST_CASE("a side of 1 stays 1 and its taps repeat the one row", "[editor][assets][mips]") {
    FloatImage source;
    source.width = 4;
    source.height = 1;
    source.pixels = {0.0f, 0.0f, 0.0f, 1.0f, 4.0f, 8.0f, 12.0f, 1.0f, 10.0f, 10.0f, 10.0f, 1.0f, 20.0f, 30.0f, 40.0f, 1.0f};

    FloatImage level;
    TextureImporter::downsample(source, level);

    REQUIRE(level.width == 2);
    REQUIRE(level.height == 1);
    CHECK(channel(level, 0, 0, 0) == Catch::Approx(2.0f));
    CHECK(channel(level, 0, 0, 1) == Catch::Approx(4.0f));
    CHECK(channel(level, 0, 0, 2) == Catch::Approx(6.0f));
    CHECK(channel(level, 0, 0, 3) == Catch::Approx(1.0f));
    CHECK(channel(level, 1, 0, 0) == Catch::Approx(15.0f));
    CHECK(channel(level, 1, 0, 1) == Catch::Approx(20.0f));
    CHECK(channel(level, 1, 0, 2) == Catch::Approx(25.0f));
}

TEST_CASE("a power-of-two image keeps its total through the whole chain", "[editor][assets][mips]") {
    const FloatImage source = makeFloatImage(8, 4, 3.0f);
    std::vector<FloatImage> chain;
    TextureImporter::buildMipChain(source, chain);

    float sums[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    for (std::uint32_t y = 0; y < 4; y++) {
        for (std::uint32_t x = 0; x < 8; x++) {
            for (std::uint32_t index = 0; index < 4; index++) {
                sums[index] += channel(source, x, y, index);
            }
        }
    }
    REQUIRE_FALSE(chain.empty());
    for (std::uint32_t index = 0; index < 4; index++) {
        CHECK(channel(chain.back(), 0, 0, index) == Catch::Approx(sums[index] / 32.0f));
    }
}

TEST_CASE("an image with a side of 0 has no chain and clears the output", "[editor][assets][mips]") {
    std::vector<FloatImage> chain(3);
    FloatImage empty;
    empty.width = 4;

    TextureImporter::buildMipChain(empty, chain);

    CHECK(chain.empty());
}

TEST_CASE("a normal map's chain keeps every texel's xyz unit length at every level", "[editor][assets][mips]") {
    // Normals that fan out across a 16x8 image, so every box filter averages directions that differ.
    FloatImage source;
    source.width = 16;
    source.height = 8;
    for (std::uint32_t y = 0; y < source.height; y++) {
        for (std::uint32_t x = 0; x < source.width; x++) {
            const float angleX = (static_cast<float>(x) - 7.5f) * 0.18f;
            const float angleY = (static_cast<float>(y) - 3.5f) * 0.3f;
            const float normalX = std::sin(angleX);
            const float normalY = std::sin(angleY) * std::cos(angleX);
            const float normalZ = std::cos(angleY) * std::cos(angleX);
            source.pixels.insert(source.pixels.end(), {normalX, normalY, normalZ, 1.0f});
        }
    }

    std::vector<FloatImage> chain;
    TextureImporter::buildNormalMapMipChain(source, chain);

    REQUIRE(chain.size() + 1 == Texture::mipCount(16, 8));
    for (const FloatImage& level: chain) {
        for (std::size_t pixel = 0; pixel < level.pixels.size(); pixel += 4) {
            const float length = std::sqrt(level.pixels[pixel] * level.pixels[pixel] + level.pixels[pixel + 1] * level.pixels[pixel + 1] + level.pixels[pixel + 2] * level.pixels[pixel + 2]);
            CHECK(length == Catch::Approx(1.0f).margin(1.0e-5f));
            CHECK(level.pixels[pixel + 3] == Catch::Approx(1.0f));
        }
    }

    SECTION("the plain box filter, by contrast, shortens them") {
        std::vector<FloatImage> plain;
        TextureImporter::buildMipChain(source, plain);
        const FloatImage& last = plain.back();
        const float length = std::sqrt(last.pixels[0] * last.pixels[0] + last.pixels[1] * last.pixels[1] + last.pixels[2] * last.pixels[2]);
        CHECK(length < 0.95f);
    }
}

TEST_CASE("a normal map texel whose xyz averages to zero becomes +Z", "[editor][assets][mips]") {
    FloatImage source;
    source.width = 2;
    source.height = 1;
    source.pixels = {1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f};

    std::vector<FloatImage> chain;
    TextureImporter::buildNormalMapMipChain(source, chain);

    REQUIRE(chain.size() == 1);
    CHECK(chain[0].pixels == std::vector<float>{0.0f, 0.0f, 1.0f, 1.0f});
}

TEST_CASE("zero packs to the all-zero word", "[editor][assets][rgb9e5]") {
    CHECK(TextureImporter::packRgb9E5(0.0f, 0.0f, 0.0f) == 0);
}

TEST_CASE("one packs to the shared exponent 16 and a mantissa of 256", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = TextureImporter::packRgb9E5(1.0f, 1.0f, 1.0f);

    CHECK(packed == ((16u << EXPONENT_SHIFT) | (256u << 18) | (256u << 9) | 256u));
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    TextureImporter::unpackRgb9E5(packed, red, green, blue);
    CHECK(red == 1.0f);
    CHECK(green == 1.0f);
    CHECK(blue == 1.0f);
}

TEST_CASE("each channel lands in its own field", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = TextureImporter::packRgb9E5(1.0f, 0.5f, 0.25f);

    CHECK(((packed >> 27) & 0x1F) == 16);
    CHECK((packed & 0x1FF) == 256);
    CHECK(((packed >> 9) & 0x1FF) == 128);
    CHECK(((packed >> 18) & 0x1FF) == 64);
}

TEST_CASE("the largest value packs to the largest exponent and mantissa", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = TextureImporter::packRgb9E5(TextureImporter::RGB9E5_MAX_VALUE, 0.0f, 0.0f);

    CHECK(packed == ((31u << EXPONENT_SHIFT) | 511u));
    float red = 0.0f;
    unpackRed(packed, red);
    CHECK(red == TextureImporter::RGB9E5_MAX_VALUE);
}

TEST_CASE("a value above the largest is clamped to it", "[editor][assets][rgb9e5]") {
    const std::uint32_t expected = TextureImporter::packRgb9E5(TextureImporter::RGB9E5_MAX_VALUE, 0.0f, 0.0f);

    CHECK(TextureImporter::packRgb9E5(65409.0f, 0.0f, 0.0f) == expected);
    CHECK(TextureImporter::packRgb9E5(1.0e30f, 0.0f, 0.0f) == expected);
    CHECK(TextureImporter::packRgb9E5(std::numeric_limits<float>::infinity(), 0.0f, 0.0f) == expected);
}

TEST_CASE("a negative or NaN channel packs as zero and does not move the other channels", "[editor][assets][rgb9e5]") {
    const std::uint32_t expected = TextureImporter::packRgb9E5(0.0f, 0.0f, 5.0f);

    CHECK(TextureImporter::packRgb9E5(-1.0f, 0.0f, 5.0f) == expected);
    CHECK(TextureImporter::packRgb9E5(std::numeric_limits<float>::quiet_NaN(), -std::numeric_limits<float>::infinity(), 5.0f) == expected);
    CHECK(TextureImporter::packRgb9E5(-3.0f, -2.0f, -1.0f) == 0);
}

TEST_CASE("a mantissa that rounds up to 512 carries into the exponent", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = TextureImporter::packRgb9E5(1.999f, 0.0f, 0.0f);

    CHECK((packed >> EXPONENT_SHIFT) == 17);
    CHECK((packed & 0x1FF) == 256);
    float red = 0.0f;
    unpackRed(packed, red);
    CHECK(red == 2.0f);
}

TEST_CASE("a small channel next to a large one keeps only the large one's precision", "[editor][assets][rgb9e5]") {
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    TextureImporter::unpackRgb9E5(TextureImporter::packRgb9E5(1000.0f, 0.4f, 0.0f), red, green, blue);

    CHECK(red == 1000.0f);
    CHECK(green == 0.0f);
    CHECK(blue == 0.0f);
}

TEST_CASE("the smallest step and values below it", "[editor][assets][rgb9e5]") {
    const float smallest = std::ldexp(1.0f, -24);

    CHECK(TextureImporter::packRgb9E5(smallest, 0.0f, 0.0f) == 1);
    CHECK(TextureImporter::packRgb9E5(std::ldexp(1.0f, -26), 0.0f, 0.0f) == 0);
    CHECK(TextureImporter::packRgb9E5(std::numeric_limits<float>::denorm_min(), 0.0f, 0.0f) == 0);
    float red = 0.0f;
    unpackRed(1, red);
    CHECK(red == smallest);
}

TEST_CASE("a round trip stays within half a step of the largest channel", "[editor][assets][rgb9e5]") {
    const float values[] = {0.001f, 0.1f, 0.333333f, 1.0f, 3.14159f, 77.7f, 1234.5f, 60000.0f};
    for (const float value: values) {
        float red = 0.0f;
        float green = 0.0f;
        float blue = 0.0f;
        TextureImporter::unpackRgb9E5(TextureImporter::packRgb9E5(value, value * 0.5f, value * 0.1f), red, green, blue);

        const float step = value / 256.0f;
        CHECK(red == Catch::Approx(value).margin(step));
        CHECK(green == Catch::Approx(value * 0.5f).margin(step));
        CHECK(blue == Catch::Approx(value * 0.1f).margin(step));
    }
}
