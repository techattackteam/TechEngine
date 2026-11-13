#include <TechEngine/core/texture/Texture.hpp>

#include <assets/MipBuilder.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using TechEngine::FloatImage;
using TechEngine::MipBuilder;
using TechEngine::Texture;

static FloatImage makeImage(const std::uint32_t width, const std::uint32_t height, const float firstValue) {
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

TEST_CASE("a 2x2 image averages to its mean in every channel", "[editor][assets][mips]") {
    FloatImage source;
    source.width = 2;
    source.height = 2;
    source.pixels = {1.0f, 10.0f, 100.0f, 1.0f, 3.0f, 20.0f, 200.0f, 0.0f, 5.0f, 30.0f, 300.0f, 1.0f, 7.0f, 40.0f, 400.0f, 0.0f};

    std::vector<FloatImage> chain;
    MipBuilder::build(source, chain);

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
        const FloatImage source = makeImage(size[0], size[1], 0.0f);
        std::vector<FloatImage> chain;
        MipBuilder::build(source, chain);

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
    const FloatImage source = makeImage(3, 3, 0.0f);

    std::vector<FloatImage> chain;
    MipBuilder::build(source, chain);

    REQUIRE(chain.size() == 1);
    CHECK(chain[0].width == 1);
    CHECK(chain[0].height == 1);
    for (std::uint32_t index = 0; index < 4; index++) {
        const float expected = (channel(source, 0, 0, index) + channel(source, 1, 0, index) + channel(source, 0, 1, index) + channel(source, 1, 1, index)) / 4.0f;
        CHECK(chain[0].pixels[index] == Catch::Approx(expected));
    }
}

TEST_CASE("each pixel of a wider level averages the 2x2 block at twice its position", "[editor][assets][mips]") {
    const FloatImage source = makeImage(5, 3, 1.0f);

    FloatImage level;
    MipBuilder::downsample(source, level);

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
    MipBuilder::downsample(source, level);

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
    const FloatImage source = makeImage(8, 4, 3.0f);
    std::vector<FloatImage> chain;
    MipBuilder::build(source, chain);

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

    MipBuilder::build(empty, chain);

    CHECK(chain.empty());
}
