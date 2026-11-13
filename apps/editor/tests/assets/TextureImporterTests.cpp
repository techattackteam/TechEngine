#include <TechEngine/core/texture/Texture.hpp>

#include <assets/ImageDecoder.hpp>
#include <assets/TextureImporter.hpp>
#include <assets/TextureTestFiles.hpp>

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>

using TechEngine::ByteImage;
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
