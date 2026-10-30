#include <edit/FieldValue.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <limits>
#include <string>

using TechEngine::describeFieldValue;
using TechEngine::FieldValue;

TEST_CASE("a one-byte integer is described as a number, never as a character", "[editor][edit]") {
    CHECK(describeFieldValue(FieldValue{std::int8_t{65}}) == "65");
    CHECK(describeFieldValue(FieldValue{std::int8_t{-8}}) == "-8");
    CHECK(describeFieldValue(FieldValue{std::uint8_t{255}}) == "255");
}

TEST_CASE("a string is described in quotes, and an empty one as two quotes", "[editor][edit]") {
    CHECK(describeFieldValue(FieldValue{std::string("Crate")}) == "\"Crate\"");
    CHECK(describeFieldValue(FieldValue{std::string()}) == "\"\"");
}

TEST_CASE("bools and the widest integers are described in full", "[editor][edit]") {
    CHECK(describeFieldValue(FieldValue{true}) == "true");
    CHECK(describeFieldValue(FieldValue{false}) == "false");
    CHECK(describeFieldValue(FieldValue{std::numeric_limits<std::uint64_t>::max()}) == "18446744073709551615");
    CHECK(describeFieldValue(FieldValue{std::numeric_limits<std::int64_t>::min()}) == "-9223372036854775808");
}
