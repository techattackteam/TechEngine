#include <TechEngine/base/stringid/Format.hpp>
#include <TechEngine/base/stringid/StringId.hpp>

#include <catch2/catch_test_macros.hpp>

#include <format>
#include <string>

TEST_CASE("an id renders as fixed-width hex", "[base][stringid][format]") {
    REQUIRE(std::format("{0}", TechEngine::StringId{""}) == "0xcbf29ce484222325");
    REQUIRE(std::format("{0}", TechEngine::StringId{"a"}) == "0xaf63dc4c8601ec8c");
}

TEST_CASE("the invalid sentinel renders as zero", "[base][stringid][format]") {
    REQUIRE(std::format("{0}", TechEngine::StringId{}) == "0x0000000000000000");
}

TEST_CASE("an id composes into a larger format string", "[base][stringid][format]") {
    REQUIRE(std::format("event {0} on frame {1}", TechEngine::StringId{"a"}, 7) == "event 0xaf63dc4c8601ec8c on frame 7");
}

// Only the runtime half is reachable — a literal spec is rejected during constant
// evaluation, which is a compile error and cannot be a test case.
TEST_CASE("a format spec is rejected", "[base][stringid][format]") {
    const TechEngine::StringId id{"a"};

    REQUIRE_THROWS_AS(std::vformat("{0:x}", std::make_format_args(id)), std::format_error);
}
