#include <TechEngine/base/stringid/StringId.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

static_assert(TechEngine::StringId{""}.value() == 0xcbf29ce484222325ULL);
static_assert(TechEngine::StringId{"a"}.value() == 0xaf63dc4c8601ec8cULL);
static_assert(TechEngine::StringId{"foobar"}.value() == 0x85944171f73967e8ULL);

// The algorithm restated for a single byte ≥ 0x80 — the published ASCII vectors above pass
// just as happily with a sign-extended XOR, so this is the case that pins unsigned bytes.
static_assert(TechEngine::StringId{"\x80"}.value() == ((TechEngine::internal::FNV_OFFSET_BASIS ^ 0x80ULL) * TechEngine::internal::FNV_PRIME));

static_assert(TechEngine::StringId{}.value() == 0);
static_assert(TechEngine::StringId{"Game.Hit"} == TechEngine::StringId{"Game.Hit"});
static_assert(TechEngine::StringId{"Game.Hit"} != TechEngine::StringId{"game.hit"});
static_assert(TechEngine::StringId::fromValue(0xcbf29ce484222325ULL) == TechEngine::StringId{""});

TEST_CASE("a runtime string hashes to the same id as the literal", "[base][stringid]") {
    const std::string tag = "Game.Hit";

    REQUIRE(TechEngine::StringId{tag} == TechEngine::StringId{"Game.Hit"});
}

TEST_CASE("fromValue round-trips a serialized id", "[base][stringid]") {
    const TechEngine::StringId hit{"Game.Hit"};
    const std::uint64_t wire = hit.value();

    REQUIRE(TechEngine::StringId::fromValue(wire) == hit);
}

TEST_CASE("ids key an unordered_map", "[base][stringid]") {
    std::unordered_map<TechEngine::StringId, int> counts;
    counts[TechEngine::StringId{"Game.Hit"}] = 3;

    REQUIRE(counts.at(TechEngine::StringId{"Game.Hit"}) == 3);
    REQUIRE(counts.find(TechEngine::StringId{"Game.Miss"}) == counts.end());
}

TEST_CASE("ids order by value", "[base][stringid]") {
    REQUIRE(TechEngine::StringId::fromValue(1) < TechEngine::StringId::fromValue(2));
}
