#include <assets/Rgb9E5.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>
#include <limits>

using TechEngine::Rgb9E5;

static constexpr std::uint32_t EXPONENT_SHIFT = 27;

static void unpackRed(const std::uint32_t packed, float& red) {
    float green = 0.0f;
    float blue = 0.0f;
    Rgb9E5::unpack(packed, red, green, blue);
}

TEST_CASE("zero packs to the all-zero word", "[editor][assets][rgb9e5]") {
    CHECK(Rgb9E5::pack(0.0f, 0.0f, 0.0f) == 0);
}

TEST_CASE("one packs to the shared exponent 16 and a mantissa of 256", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = Rgb9E5::pack(1.0f, 1.0f, 1.0f);

    CHECK(packed == ((16u << EXPONENT_SHIFT) | (256u << 18) | (256u << 9) | 256u));
    float red = 0.0f;
    float green = 0.0f;
    float blue = 0.0f;
    Rgb9E5::unpack(packed, red, green, blue);
    CHECK(red == 1.0f);
    CHECK(green == 1.0f);
    CHECK(blue == 1.0f);
}

TEST_CASE("each channel lands in its own field", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = Rgb9E5::pack(1.0f, 0.5f, 0.25f);

    CHECK(((packed >> 27) & 0x1F) == 16);
    CHECK((packed & 0x1FF) == 256);
    CHECK(((packed >> 9) & 0x1FF) == 128);
    CHECK(((packed >> 18) & 0x1FF) == 64);
}

TEST_CASE("the largest value packs to the largest exponent and mantissa", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = Rgb9E5::pack(Rgb9E5::MAX_VALUE, 0.0f, 0.0f);

    CHECK(packed == ((31u << EXPONENT_SHIFT) | 511u));
    float red = 0.0f;
    unpackRed(packed, red);
    CHECK(red == Rgb9E5::MAX_VALUE);
}

TEST_CASE("a value above the largest is clamped to it", "[editor][assets][rgb9e5]") {
    const std::uint32_t expected = Rgb9E5::pack(Rgb9E5::MAX_VALUE, 0.0f, 0.0f);

    CHECK(Rgb9E5::pack(65409.0f, 0.0f, 0.0f) == expected);
    CHECK(Rgb9E5::pack(1.0e30f, 0.0f, 0.0f) == expected);
    CHECK(Rgb9E5::pack(std::numeric_limits<float>::infinity(), 0.0f, 0.0f) == expected);
}

TEST_CASE("a negative or NaN channel packs as zero and does not move the other channels", "[editor][assets][rgb9e5]") {
    const std::uint32_t expected = Rgb9E5::pack(0.0f, 0.0f, 5.0f);

    CHECK(Rgb9E5::pack(-1.0f, 0.0f, 5.0f) == expected);
    CHECK(Rgb9E5::pack(std::numeric_limits<float>::quiet_NaN(), -std::numeric_limits<float>::infinity(), 5.0f) == expected);
    CHECK(Rgb9E5::pack(-3.0f, -2.0f, -1.0f) == 0);
}

TEST_CASE("a mantissa that rounds up to 512 carries into the exponent", "[editor][assets][rgb9e5]") {
    const std::uint32_t packed = Rgb9E5::pack(1.999f, 0.0f, 0.0f);

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
    Rgb9E5::unpack(Rgb9E5::pack(1000.0f, 0.4f, 0.0f), red, green, blue);

    CHECK(red == 1000.0f);
    CHECK(green == 0.0f);
    CHECK(blue == 0.0f);
}

TEST_CASE("the smallest step and values below it", "[editor][assets][rgb9e5]") {
    const float smallest = std::ldexp(1.0f, -24);

    CHECK(Rgb9E5::pack(smallest, 0.0f, 0.0f) == 1);
    CHECK(Rgb9E5::pack(std::ldexp(1.0f, -26), 0.0f, 0.0f) == 0);
    CHECK(Rgb9E5::pack(std::numeric_limits<float>::denorm_min(), 0.0f, 0.0f) == 0);
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
        Rgb9E5::unpack(Rgb9E5::pack(value, value * 0.5f, value * 0.1f), red, green, blue);

        const float step = value / 256.0f;
        CHECK(red == Catch::Approx(value).margin(step));
        CHECK(green == Catch::Approx(value * 0.5f).margin(step));
        CHECK(blue == Catch::Approx(value * 0.1f).margin(step));
    }
}
