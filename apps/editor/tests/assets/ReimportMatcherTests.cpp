#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <assets/ReimportMatcher.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

using TechEngine::ImportKey;
using TechEngine::REIMPORT_NO_MATCH;
using TechEngine::ReimportMatcher;

static ImportKey meshKey(const std::uint32_t index, const std::string& name) {
    return ImportKey{TechEngine::MESH_TYPE_TAG, index, name};
}

static std::vector<std::uint32_t> match(const std::vector<ImportKey>& oldKeys, const std::vector<ImportKey>& wanted) {
    std::vector<std::uint32_t> out;
    ReimportMatcher::match(oldKeys, wanted, out);
    REQUIRE(out.size() == wanted.size());
    return out;
}

TEST_CASE("unchanged keys each keep their own old file", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> keys{meshKey(0, "Hull"), meshKey(1, "Tire"), meshKey(2, "")};

    CHECK(match(keys, keys) == std::vector<std::uint32_t>{0, 1, 2});
}

TEST_CASE("a name matches before an index, so reordered outputs keep their named files", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> oldKeys{meshKey(0, "Hull"), meshKey(1, "Tire")};

    CHECK(match(oldKeys, {meshKey(0, "Tire"), meshKey(1, "Hull")}) == std::vector<std::uint32_t>{1, 0});
}

TEST_CASE("among equal names the same index wins, whatever order the outputs come in", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> oldKeys{meshKey(0, "Paint"), meshKey(1, "Paint"), meshKey(2, "Paint")};

    SECTION("in index order") {
        CHECK(match(oldKeys, {meshKey(0, "Paint"), meshKey(1, "Paint"), meshKey(2, "Paint")}) == std::vector<std::uint32_t>{0, 1, 2});
    }

    SECTION("an output whose index moved takes no file an exact output owns") {
        CHECK(match(oldKeys, {meshKey(5, "Paint"), meshKey(2, "Paint")}) == std::vector<std::uint32_t>{0, 2});
    }

    SECTION("with no exact index, the lowest index left is taken first") {
        CHECK(match({meshKey(4, "Paint"), meshKey(2, "Paint")}, {meshKey(7, "Paint"), meshKey(8, "Paint")}) == std::vector<std::uint32_t>{1, 0});
    }
}

TEST_CASE("an empty name matches an empty name at the same index alone", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> oldKeys{meshKey(0, ""), meshKey(1, ""), meshKey(2, "Named")};

    SECTION("by index") {
        CHECK(match(oldKeys, {meshKey(1, ""), meshKey(0, "")}) == std::vector<std::uint32_t>{1, 0});
    }

    SECTION("an empty name at a new index is new") {
        CHECK(match(oldKeys, {meshKey(5, "")}) == std::vector<std::uint32_t>{REIMPORT_NO_MATCH});
    }

    SECTION("an empty name never takes a named file at its index, nor a name an unnamed one") {
        CHECK(match(oldKeys, {meshKey(2, ""), meshKey(0, "Named0")}) == std::vector<std::uint32_t>{REIMPORT_NO_MATCH, REIMPORT_NO_MATCH});
    }
}

TEST_CASE("a key matches only an old key of its own type", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> oldKeys{ImportKey{TechEngine::MATERIAL_TYPE_TAG, 0, "Paint"}, ImportKey{TechEngine::TEXTURE_TYPE_TAG, 0, "wall.png#basecolor#repeat-linear"}};

    CHECK(match(oldKeys, {meshKey(0, "Paint"), ImportKey{TechEngine::TEXTURE_TYPE_TAG, 0, "wall.png#basecolor#repeat-linear"}, ImportKey{TechEngine::MATERIAL_TYPE_TAG, 0, "Paint"}}) == std::vector<std::uint32_t>{REIMPORT_NO_MATCH, 1, 0});
}

TEST_CASE("an old file is kept once, and an output beyond the old files is new", "[editor][assets][reimport][matcher]") {
    const std::vector<ImportKey> oldKeys{meshKey(0, "Hull")};

    CHECK(match(oldKeys, {meshKey(0, "Hull"), meshKey(0, "Hull"), meshKey(3, "Hull")}) == std::vector<std::uint32_t>{0, REIMPORT_NO_MATCH, REIMPORT_NO_MATCH});
}

TEST_CASE("no old files or no outputs match nothing", "[editor][assets][reimport][matcher]") {
    CHECK(match({}, {meshKey(0, "Hull")}) == std::vector<std::uint32_t>{REIMPORT_NO_MATCH});
    CHECK(match({meshKey(0, "Hull")}, {}).empty());
}
