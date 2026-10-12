#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetIndex;
using TechEngine::AssetRecord;
using TechEngine::BlobKind;
using TechEngine::DuplicateAsset;
using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::MountTable;
using TechEngine::ResourceId;
using TechEngine::SCENE_KIND;
using TechEngine::SCENE_TYPE_TAG;
using TechEngine::Side;
using TechEngineTests::assetBytes;
using TechEngineTests::ScratchDirectory;
using TechEngineTests::writeBytes;

static constexpr std::string_view INDEX_ASSETS_ROOT = "project://assets";

static constexpr ResourceId COMMON_SCENE_ID{0x1000000000000001};
static constexpr ResourceId CLIENT_SCENE_ID{0x2000000000000002};
static constexpr ResourceId SERVER_SCENE_ID{0x3000000000000003};
static constexpr ResourceId NESTED_SCENE_ID{0x4000000000000004};
static constexpr ResourceId DUPLICATED_ID{0x5000000000000005};

class IndexScratch {
public:
    explicit IndexScratch(const std::string_view name) : m_scratch(name), m_files(m_mounts) {
        m_scratch.makeDirectory("assets/common");
        m_scratch.makeDirectory("assets/client");
        m_scratch.makeDirectory("assets/server");
        m_mounts.mount("project", m_scratch.root());
    }

    const ScratchDirectory& scratch() const {
        return m_scratch;
    }

    const FileAccess& files() const {
        return m_files;
    }

    void writeScene(const std::filesystem::path& relative, const ResourceId id) const {
        writeBytes(m_scratch, relative, assetBytes(SCENE_KIND, id, "scene payload"));
    }

private:
    ScratchDirectory m_scratch;
    MountTable m_mounts;
    FileAccess m_files;
};

TEST_CASE("build records each asset's type tag, path and side on all three sides", "[editor][assets]") {
    const IndexScratch env{"assetIndexSides"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/client/menu.tescene", CLIENT_SCENE_ID);
    env.writeScene("assets/server/arena.tescene", SERVER_SCENE_ID);

    AssetIndex index;
    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

    CHECK(index.getRecords().size() == 3);
    CHECK(index.getDuplicates().empty());

    AssetRecord common;
    REQUIRE(index.find(COMMON_SCENE_ID, common));
    CHECK(common.id == COMMON_SCENE_ID);
    CHECK(common.typeTag == SCENE_TYPE_TAG);
    CHECK(common.side == Side::Common);
    CHECK(common.path == "project://assets/common/level.tescene");

    AssetRecord client;
    REQUIRE(index.find(CLIENT_SCENE_ID, client));
    CHECK(client.side == Side::Client);
    CHECK(client.path == "project://assets/client/menu.tescene");

    AssetRecord server;
    REQUIRE(index.find(SERVER_SCENE_ID, server));
    CHECK(server.side == Side::Server);
    CHECK(server.path == "project://assets/server/arena.tescene");
}

TEST_CASE("build reads assets in subfolders of a side", "[editor][assets]") {
    const IndexScratch env{"assetIndexNested"};
    env.writeScene("assets/client/levels/deep/nested.tescene", NESTED_SCENE_ID);

    AssetIndex index;
    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

    AssetRecord record;
    REQUIRE(index.find(NESTED_SCENE_ID, record));
    CHECK(record.side == Side::Client);
    CHECK(record.path == "project://assets/client/levels/deep/nested.tescene");
}

TEST_CASE("build skips every file that is not an asset it knows and still succeeds", "[editor][assets]") {
    const IndexScratch env{"assetIndexSkips"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);

    env.scratch().writeFile("assets/client/shaders/GeometryShader.vert", "#version 450 core\nvoid main() {}\n");
    env.scratch().writeFile("assets/client/.gitkeep", "");
    std::vector<std::byte> headerOnly = assetBytes(SCENE_KIND, ResourceId(0x8000000000000008), "");
    headerOnly.resize(8);
    writeBytes(env.scratch(), "assets/common/header-only.tescene", headerOnly);
    writeBytes(env.scratch(), "assets/common/unknown-kind.bin", assetBytes(BlobKind{0x12345678, 1}, ResourceId(0x6000000000000006), "x"));
    writeBytes(env.scratch(), "assets/common/newer.tescene", assetBytes(BlobKind{SCENE_KIND.magic, static_cast<std::uint16_t>(SCENE_KIND.version + 1)}, ResourceId(0x7000000000000007), "x"));
    writeBytes(env.scratch(), "assets/common/null-id.tescene", assetBytes(SCENE_KIND, ResourceId(), "x"));

    AssetIndex index;
    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

    REQUIRE(index.getRecords().size() == 1);
    CHECK(index.getRecords().front().id == COMMON_SCENE_ID);
    CHECK(index.getDuplicates().empty());

    AssetRecord record;
    CHECK_FALSE(index.find(ResourceId(0x6000000000000006), record));
    CHECK_FALSE(index.find(ResourceId(0x7000000000000007), record));
    CHECK_FALSE(index.find(ResourceId(0x8000000000000008), record));
    CHECK_FALSE(index.find(ResourceId(), record));
}

TEST_CASE("a duplicate id is reported with both paths and indexed under neither", "[editor][assets]") {
    const IndexScratch env{"assetIndexDuplicates"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);

    SECTION("across two sides") {
        env.writeScene("assets/common/original.tescene", DUPLICATED_ID);
        env.writeScene("assets/client/copy.tescene", DUPLICATED_ID);

        AssetIndex index;
        REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

        REQUIRE(index.getDuplicates().size() == 1);
        const DuplicateAsset& duplicate = index.getDuplicates().front();
        CHECK(duplicate.id == DUPLICATED_ID);
        const bool inOrder = duplicate.firstPath == "project://assets/common/original.tescene" && duplicate.secondPath == "project://assets/client/copy.tescene";
        const bool reversed = duplicate.firstPath == "project://assets/client/copy.tescene" && duplicate.secondPath == "project://assets/common/original.tescene";
        CHECK((inOrder || reversed));

        AssetRecord record;
        CHECK_FALSE(index.find(DUPLICATED_ID, record));
        CHECK(index.find(COMMON_SCENE_ID, record));
        CHECK(index.getRecords().size() == 1);
    }

    SECTION("within one side") {
        env.writeScene("assets/server/a.tescene", DUPLICATED_ID);
        env.writeScene("assets/server/b.tescene", DUPLICATED_ID);

        AssetIndex index;
        REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

        REQUIRE(index.getDuplicates().size() == 1);
        CHECK(index.getDuplicates().front().id == DUPLICATED_ID);

        AssetRecord record;
        CHECK_FALSE(index.find(DUPLICATED_ID, record));
        CHECK(index.getRecords().size() == 1);
    }
}

TEST_CASE("build on a project with empty sides indexes nothing", "[editor][assets]") {
    const IndexScratch env{"assetIndexEmpty"};

    AssetIndex index;
    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});

    CHECK(index.getRecords().empty());
    CHECK(index.getDuplicates().empty());
}

TEST_CASE("build returns the listing error when a side folder is missing", "[editor][assets]") {
    const IndexScratch env{"assetIndexMissingSide"};
    std::filesystem::remove_all(env.scratch().root() / "assets" / "server");

    AssetIndex index;
    CHECK(index.build(env.files(), INDEX_ASSETS_ROOT) == FileError::NotFound);
}
