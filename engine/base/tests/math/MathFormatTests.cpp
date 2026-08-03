#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Format.hpp>
#include <TechEngine/base/math/Math.hpp>

#include <catch2/catch_test_macros.hpp>

#include <format>
#include <string>

TEST_CASE("a vector renders as its glm type name and elements", "[base][math][format]") {
    REQUIRE(std::format("{0}", TechEngine::Vec2{1.0f, 2.5f}) == "vec2(1, 2.5)");
    REQUIRE(std::format("{0}", TechEngine::Vec3{1.0f, 2.5f, 3.0f}) == "vec3(1, 2.5, 3)");
    REQUIRE(std::format("{0}", TechEngine::Vec4{1.0f, 2.5f, 3.0f, 4.0f}) == "vec4(1, 2.5, 3, 4)");
}

TEST_CASE("integer and unsigned vectors carry glm's type prefix", "[base][math][format]") {
    REQUIRE(std::format("{0}", TechEngine::IVec3{-1, 2, 3}) == "ivec3(-1, 2, 3)");
    REQUIRE(std::format("{0}", TechEngine::UVec2{1u, 2u}) == "uvec2(1, 2)");
}

TEST_CASE("the format spec forwards to the elements", "[base][math][format]") {
    REQUIRE(std::format("{0:.2f}", TechEngine::Vec3{1.0f, 2.5f, 3.0f}) == "vec3(1.00, 2.50, 3.00)");
    REQUIRE(std::format("{0:03}", TechEngine::IVec2{1, 2}) == "ivec2(001, 002)");
}

TEST_CASE("a square matrix renders as columns", "[base][math][format]") {
    REQUIRE(std::format("{0}", TechEngine::Mat4{1.0f}) == "mat4((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0), (0, 0, 0, 1))");
    REQUIRE(std::format("{0}", TechEngine::Mat3{1.0f}) == "mat3((1, 0, 0), (0, 1, 0), (0, 0, 1))");
}

TEST_CASE("a non-square matrix renders as columns", "[base][math][format]") {
    REQUIRE(std::format("{0}", TechEngine::Mat4x3{1.0f}) == "mat4x3((1, 0, 0), (0, 1, 0), (0, 0, 1), (0, 0, 0))");
    REQUIRE(std::format("{0}", TechEngine::Mat3x4{1.0f}) == "mat3x4((1, 0, 0, 0), (0, 1, 0, 0), (0, 0, 1, 0))");
}

TEST_CASE("a quaternion renders xyzw", "[base][math][format]") {
    REQUIRE(std::format("{0}", TechEngine::Quat{1.0f, 0.0f, 0.0f, 0.0f}) == "quat(0, 0, 0, 1)");
    REQUIRE(std::format("{0}", TechEngine::Quat{0.5f, 0.5f, 0.0f, 0.0f}) == "quat(0.5, 0, 0, 0.5)");
}

TEST_CASE("a vector composes into a larger format string", "[base][math][format]") {
    const TechEngine::Vec3 position{1.0f, 2.0f, 3.0f};

    REQUIRE(std::format("at {0} on frame {1}", position, 7) == "at vec3(1, 2, 3) on frame 7");
}

TEST_CASE("a vector logs through the positional format string", "[base][math][format]") {
    const TechEngine::Vec3 position{1.0f, 2.0f, 3.0f};

    TE_LOGGER_INFO("spawned at {0}", position);
}
