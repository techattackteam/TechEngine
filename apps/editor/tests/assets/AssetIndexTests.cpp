#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetIndex;
using TechEngine::AssetRecord;
using TechEngine::BlobKind;
using TechEngine::DuplicateAsset;
using TechEngine::FileAccess;
using TechEngine::FileError;
using TechEngine::ListedEntry;
using TechEngine::ListedKind;
using TechEngine::MATERIAL_KIND;
using TechEngine::MATERIAL_TYPE_TAG;
using TechEngine::MESH_KIND;
using TechEngine::MESH_TYPE_TAG;
using TechEngine::MODEL_KIND;
using TechEngine::MODEL_TYPE_TAG;
using TechEngine::MountTable;
using TechEngine::ResourceId;
using TechEngine::SCENE_KIND;
using TechEngine::SCENE_TYPE_TAG;
using TechEngine::Side;
using TechEngine::TEXTURE_KIND;
using TechEngine::TEXTURE_TYPE_TAG;
using TechEngineTests::assetBytes;
using TechEngineTests::ScratchDirectory;
using TechEngineTests::writeBytes;

static constexpr std::string_view INDEX_ASSETS_ROOT = "project://assets";

static constexpr ResourceId COMMON_SCENE_ID{0x1000000000000001};
static constexpr ResourceId CLIENT_SCENE_ID{0x2000000000000002};
static constexpr ResourceId SERVER_SCENE_ID{0x3000000000000003};
static constexpr ResourceId NESTED_SCENE_ID{0x4000000000000004};
static constexpr ResourceId DUPLICATED_ID{0x5000000000000005};
static constexpr ResourceId COMMON_MESH_ID{0x7100000000000071};
static constexpr ResourceId COMMON_MODEL_ID{0x7200000000000072};
static constexpr ResourceId COMMON_TEXTURE_ID{0x7300000000000073};
static constexpr ResourceId COMMON_MATERIAL_ID{0x7400000000000074};

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

    AssetIndex buildIndex() const {
        AssetIndex index;
        REQUIRE(index.build(m_files, INDEX_ASSETS_ROOT) == std::error_code{});
        return index;
    }

    void writeScene(const std::filesystem::path& relative, const ResourceId id) const {
        writeBytes(m_scratch, relative, assetBytes(SCENE_KIND, id, "scene payload"));
    }

private:
    ScratchDirectory m_scratch;
    MountTable m_mounts;
    FileAccess m_files;
};

static const ListedEntry* findListed(const AssetIndex& index, const std::string_view path) {
    const std::vector<ListedEntry>& listing = index.getListing();
    const auto found = std::ranges::find(listing, path, &ListedEntry::path);
    return found == listing.end() ? nullptr : &*found;
}

static bool loggedSkip(const TechEngineTests::LogCaptureGuard& logs, const std::string_view path) {
    return logs.contains("Asset index: skipped " + std::string(path));
}

TEST_CASE("build records each asset's type tag, path and side on all three sides", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexSides"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/client/menu.tescene", CLIENT_SCENE_ID);
    env.writeScene("assets/server/arena.tescene", SERVER_SCENE_ID);

    const AssetIndex index = env.buildIndex();

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

TEST_CASE("build records a mesh under the mesh type tag", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexMesh"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    writeBytes(env.scratch(), "assets/common/triangle.temesh", assetBytes(MESH_KIND, COMMON_MESH_ID, "mesh payload"));

    const AssetIndex index = env.buildIndex();

    CHECK(index.getRecords().size() == 2);
    AssetRecord mesh;
    REQUIRE(index.find(COMMON_MESH_ID, mesh));
    CHECK(mesh.typeTag == MESH_TYPE_TAG);
    CHECK(mesh.side == Side::Common);
    CHECK(mesh.path == "project://assets/common/triangle.temesh");
}

TEST_CASE("build records a model in its import folder under the model type tag", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexModel"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    writeBytes(env.scratch(), "assets/common/Kart/Kart.temodel", assetBytes(MODEL_KIND, COMMON_MODEL_ID, "model payload"));

    const AssetIndex index = env.buildIndex();

    CHECK(index.getRecords().size() == 2);
    AssetRecord model;
    REQUIRE(index.find(COMMON_MODEL_ID, model));
    CHECK(model.typeTag == MODEL_TYPE_TAG);
    CHECK(model.side == Side::Common);
    CHECK(model.path == "project://assets/common/Kart/Kart.temodel");
}

TEST_CASE("build records a texture under the texture type tag", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexTexture"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    writeBytes(env.scratch(), "assets/common/Textures/wall.tetexture", assetBytes(TEXTURE_KIND, COMMON_TEXTURE_ID, "texture payload"));

    const AssetIndex index = env.buildIndex();

    CHECK(index.getRecords().size() == 2);
    AssetRecord texture;
    REQUIRE(index.find(COMMON_TEXTURE_ID, texture));
    CHECK(texture.typeTag == TEXTURE_TYPE_TAG);
    CHECK(texture.side == Side::Common);
    CHECK(texture.path == "project://assets/common/Textures/wall.tetexture");
}

TEST_CASE("build records a material under the material type tag", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexMaterial"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    writeBytes(env.scratch(), "assets/common/Materials/bricks.tematerial", assetBytes(MATERIAL_KIND, COMMON_MATERIAL_ID, "material payload"));

    const AssetIndex index = env.buildIndex();

    CHECK(index.getRecords().size() == 2);
    AssetRecord material;
    REQUIRE(index.find(COMMON_MATERIAL_ID, material));
    CHECK(material.typeTag == MATERIAL_TYPE_TAG);
    CHECK(material.side == Side::Common);
    CHECK(material.path == "project://assets/common/Materials/bricks.tematerial");
}

TEST_CASE("build reads assets in subfolders of a side", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexNested"};
    env.writeScene("assets/client/levels/deep/nested.tescene", NESTED_SCENE_ID);

    const AssetIndex index = env.buildIndex();

    AssetRecord record;
    REQUIRE(index.find(NESTED_SCENE_ID, record));
    CHECK(record.side == Side::Client);
    CHECK(record.path == "project://assets/client/levels/deep/nested.tescene");
}

TEST_CASE("build skips every file that is not an asset it knows and still succeeds", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexSkips"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);

    env.scratch().writeFile("assets/client/shaders/GeometryShader.vert", "#version 460 core\nvoid main() {}\n");
    env.scratch().writeFile("assets/client/.gitkeep", "");
    std::vector<std::byte> headerOnly = assetBytes(SCENE_KIND, ResourceId(0x8000000000000008), "");
    headerOnly.resize(8);
    writeBytes(env.scratch(), "assets/common/header-only.tescene", headerOnly);
    writeBytes(env.scratch(), "assets/common/unknown-kind.bin", assetBytes(BlobKind{0x12345678, 1}, ResourceId(0x6000000000000006), "x"));
    writeBytes(env.scratch(), "assets/common/newer.tescene", assetBytes(BlobKind{SCENE_KIND.magic, static_cast<std::uint16_t>(SCENE_KIND.version + 1)}, ResourceId(0x7000000000000007), "x"));
    writeBytes(env.scratch(), "assets/common/null-id.tescene", assetBytes(SCENE_KIND, ResourceId(), "x"));

    const AssetIndex index = env.buildIndex();

    REQUIRE(index.getRecords().size() == 1);
    CHECK(index.getRecords().front().id == COMMON_SCENE_ID);
    CHECK(index.getDuplicates().empty());

    AssetRecord record;
    CHECK_FALSE(index.find(ResourceId(0x6000000000000006), record));
    CHECK_FALSE(index.find(ResourceId(0x7000000000000007), record));
    CHECK_FALSE(index.find(ResourceId(0x8000000000000008), record));
    CHECK_FALSE(index.find(ResourceId(), record));
}

TEST_CASE("a duplicate id is reported with both paths and indexed under neither", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexDuplicates"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);

    SECTION("across two sides") {
        env.writeScene("assets/common/original.tescene", DUPLICATED_ID);
        env.writeScene("assets/client/copy.tescene", DUPLICATED_ID);

        const AssetIndex index = env.buildIndex();

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

        const AssetIndex index = env.buildIndex();

        REQUIRE(index.getDuplicates().size() == 1);
        CHECK(index.getDuplicates().front().id == DUPLICATED_ID);

        AssetRecord record;
        CHECK_FALSE(index.find(DUPLICATED_ID, record));
        CHECK(index.getRecords().size() == 1);
    }
}

TEST_CASE("build on a project with empty sides indexes nothing", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexEmpty"};

    const AssetIndex index = env.buildIndex();

    CHECK(index.getRecords().empty());
    CHECK(index.getDuplicates().empty());
}

TEST_CASE("build returns the listing error when a side folder is missing", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexMissingSide"};
    std::filesystem::remove_all(env.scratch().root() / "assets" / "server");

    AssetIndex index;
    CHECK(index.build(env.files(), INDEX_ASSETS_ROOT) == FileError::NotFound);
}

TEST_CASE("generateId returns a non-null id that the index does not hold", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexGenerateId"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/common/original.tescene", DUPLICATED_ID);
    env.writeScene("assets/client/copy.tescene", DUPLICATED_ID);

    AssetIndex index = env.buildIndex();

    for (int i = 0; i < 256; i++) {
        const ResourceId id = index.generateId();
        AssetRecord record;
        REQUIRE(id.valid());
        REQUIRE_FALSE(index.find(id, record));
        REQUIRE(id != DUPLICATED_ID);
    }
}

TEST_CASE("add keeps the records sorted by id and findable", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexAdd"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/client/menu.tescene", CLIENT_SCENE_ID);

    AssetIndex index = env.buildIndex();

    const AssetRecord last{ResourceId(0x9000000000000009), SCENE_TYPE_TAG, Side::Server, "project://assets/server/last.tescene"};
    const AssetRecord first{ResourceId(0x0000000000000001), SCENE_TYPE_TAG, Side::Common, "project://assets/common/first.tescene"};
    const AssetRecord between{ResourceId(0x1800000000000018), SCENE_TYPE_TAG, Side::Client, "project://assets/client/between.tescene"};
    index.add(last);
    index.add(first);
    index.add(between);

    REQUIRE(index.getRecords().size() == 5);
    CHECK(std::ranges::is_sorted(index.getRecords(), {}, &AssetRecord::id));

    for (const AssetRecord& added: {last, first, between}) {
        AssetRecord found;
        REQUIRE(index.find(added.id, found));
        CHECK(found.typeTag == added.typeTag);
        CHECK(found.side == added.side);
        CHECK(found.path == added.path);
    }

    AssetRecord built;
    REQUIRE(index.find(COMMON_SCENE_ID, built));
    CHECK(built.path == "project://assets/common/level.tescene");
    REQUIRE(index.find(CLIENT_SCENE_ID, built));
    CHECK(built.path == "project://assets/client/menu.tescene");
}

TEST_CASE("add on an index that was never built holds the one record", "[editor][assets]") {
    AssetIndex index;
    const AssetRecord record{COMMON_SCENE_ID, SCENE_TYPE_TAG, Side::Common, "project://assets/common/level.tescene"};

    index.add(record);

    REQUIRE(index.getRecords().size() == 1);
    AssetRecord found;
    REQUIRE(index.find(COMMON_SCENE_ID, found));
    CHECK(found.path == record.path);
}

TEST_CASE("add rejects a null, indexed or duplicated id and leaves the index unchanged", "[editor][assets][os]") {
    const IndexScratch env{"assetIndexAddRejects"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/common/original.tescene", DUPLICATED_ID);
    env.writeScene("assets/client/copy.tescene", DUPLICATED_ID);

    AssetIndex index = env.buildIndex();
    REQUIRE(index.getRecords().size() == 1);
    REQUIRE(index.getDuplicates().size() == 1);
    const TechEngineTests::FatalAssertGuard guard;

    SECTION("a null id") {
        const AssetRecord nullId{ResourceId(), SCENE_TYPE_TAG, Side::Common, "project://assets/common/null.tescene"};
        REQUIRE_THROWS_AS(index.add(nullId), TechEngineTests::AssertFired);
    }

    SECTION("an id already indexed") {
        const AssetRecord indexed{COMMON_SCENE_ID, SCENE_TYPE_TAG, Side::Client, "project://assets/client/again.tescene"};
        REQUIRE_THROWS_AS(index.add(indexed), TechEngineTests::AssertFired);
    }

    SECTION("an id reported as a duplicate") {
        const AssetRecord duplicated{DUPLICATED_ID, SCENE_TYPE_TAG, Side::Common, "project://assets/common/third.tescene"};
        REQUIRE_THROWS_AS(index.add(duplicated), TechEngineTests::AssertFired);
    }

    REQUIRE(index.getRecords().size() == 1);
    CHECK(index.getRecords().front().path == "project://assets/common/level.tescene");
    CHECK(index.getDuplicates().size() == 1);
    AssetRecord record;
    CHECK_FALSE(index.find(DUPLICATED_ID, record));
}

TEST_CASE("build lists every folder and file under the three sides with its side", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexListsAll"};
    env.writeScene("assets/common/levels/level.tescene", COMMON_SCENE_ID);
    env.scratch().makeDirectory("assets/client/empty");
    env.scratch().writeFile("assets/client/notes.txt", "notes");
    env.writeScene("assets/server/arena.tescene", SERVER_SCENE_ID);

    const AssetIndex index = env.buildIndex();

    CHECK(index.getListing().size() == 5);
    CHECK(std::ranges::is_sorted(index.getListing(), {}, &ListedEntry::path));

    const ListedEntry* levels = findListed(index, "project://assets/common/levels");
    REQUIRE(levels != nullptr);
    CHECK(levels->kind == ListedKind::Folder);
    CHECK(levels->side == Side::Common);
    CHECK_FALSE(levels->id.valid());

    const ListedEntry* empty = findListed(index, "project://assets/client/empty");
    REQUIRE(empty != nullptr);
    CHECK(empty->kind == ListedKind::Folder);
    CHECK(empty->side == Side::Client);

    const ListedEntry* level = findListed(index, "project://assets/common/levels/level.tescene");
    REQUIRE(level != nullptr);
    CHECK(level->side == Side::Common);

    const ListedEntry* notes = findListed(index, "project://assets/client/notes.txt");
    REQUIRE(notes != nullptr);
    CHECK(notes->side == Side::Client);

    const ListedEntry* arena = findListed(index, "project://assets/server/arena.tescene");
    REQUIRE(arena != nullptr);
    CHECK(arena->side == Side::Server);
}

TEST_CASE("build gives an indexed asset its id and type tag, a source its kind, and any other file none", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexListedKinds"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    writeBytes(env.scratch(), "assets/client/Kart/Kart.temodel", assetBytes(MODEL_KIND, COMMON_MODEL_ID, "model payload"));
    for (const std::string_view source: {"Kart.gltf", "Kart.glb", "Kart.fbx", "Kart.obj", "Upper.GLTF"}) {
        env.scratch().writeFile("assets/client/" + std::string(source), "not read as a model");
    }
    env.scratch().writeFile("assets/client/Sky.hdr", "not read as an image");
    env.scratch().writeFile("assets/client/Wall.png", "not read as an image");
    env.scratch().writeFile("assets/client/Kart.bin", "buffer");

    const AssetIndex index = env.buildIndex();

    const ListedEntry* level = findListed(index, "project://assets/common/level.tescene");
    REQUIRE(level != nullptr);
    CHECK(level->kind == ListedKind::Asset);
    CHECK(level->id == COMMON_SCENE_ID);
    CHECK(level->typeTag == SCENE_TYPE_TAG);

    const ListedEntry* model = findListed(index, "project://assets/client/Kart/Kart.temodel");
    REQUIRE(model != nullptr);
    CHECK(model->kind == ListedKind::Asset);
    CHECK(model->id == COMMON_MODEL_ID);
    CHECK(model->typeTag == MODEL_TYPE_TAG);
    CHECK(model->side == Side::Client);

    for (const std::string_view source: {"Kart.gltf", "Kart.glb", "Kart.fbx", "Kart.obj", "Upper.GLTF"}) {
        const ListedEntry* entry = findListed(index, "project://assets/client/" + std::string(source));
        REQUIRE(entry != nullptr);
        CHECK(entry->kind == ListedKind::ModelSource);
        CHECK_FALSE(entry->id.valid());
    }

    const ListedEntry* sky = findListed(index, "project://assets/client/Sky.hdr");
    REQUIRE(sky != nullptr);
    CHECK(sky->kind == ListedKind::SkySource);

    for (const std::string_view other: {"Wall.png", "Kart.bin"}) {
        const ListedEntry* entry = findListed(index, "project://assets/client/" + std::string(other));
        REQUIRE(entry != nullptr);
        CHECK(entry->kind == ListedKind::Other);
        CHECK_FALSE(entry->id.valid());
        CHECK(entry->typeTag == TechEngine::StringId{});
    }
}

TEST_CASE("a file the index drops is listed with no kind", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexListsDropped"};
    env.writeScene("assets/common/original.tescene", DUPLICATED_ID);
    env.writeScene("assets/client/copy.tescene", DUPLICATED_ID);
    writeBytes(env.scratch(), "assets/common/null-id.tescene", assetBytes(SCENE_KIND, ResourceId(), "x"));

    const AssetIndex index = env.buildIndex();

    REQUIRE(index.getDuplicates().size() == 1);
    for (const std::string_view path: {"project://assets/common/original.tescene", "project://assets/client/copy.tescene", "project://assets/common/null-id.tescene"}) {
        const ListedEntry* entry = findListed(index, path);
        REQUIRE(entry != nullptr);
        CHECK(entry->kind == ListedKind::Other);
        CHECK_FALSE(entry->id.valid());
    }
}

TEST_CASE("each successful build raises the listing revision", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexRevision"};
    AssetIndex index;
    CHECK(index.getListingRevision() == 0);
    CHECK(index.getListing().empty());

    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});
    CHECK(index.getListingRevision() == 1);

    REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == std::error_code{});
    CHECK(index.getListingRevision() == 2);

    REQUIRE(index.rebuild(env.files()) == std::error_code{});
    CHECK(index.getListingRevision() == 3);
}

TEST_CASE("a failed build keeps the records, the listing and the revision of the last one that succeeded", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexFailedRebuild"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/client/menu.tescene", CLIENT_SCENE_ID);
    AssetIndex index = env.buildIndex();
    REQUIRE(index.getRecords().size() == 2);
    const std::size_t listed = index.getListing().size();
    const std::uint64_t revision = index.getListingRevision();

    env.writeScene("assets/common/later.tescene", NESTED_SCENE_ID);
    std::filesystem::remove_all(env.scratch().root() / "assets" / "server");

    CHECK(index.rebuild(env.files()) == FileError::NotFound);

    CHECK(index.getRecords().size() == 2);
    AssetRecord record;
    CHECK(index.find(COMMON_SCENE_ID, record));
    CHECK(index.find(CLIENT_SCENE_ID, record));
    CHECK_FALSE(index.find(NESTED_SCENE_ID, record));
    CHECK(index.getListing().size() == listed);
    CHECK(findListed(index, "project://assets/common/later.tescene") == nullptr);
    CHECK(index.getListingRevision() == revision);

    SECTION("and the next build that succeeds picks up what the failed one missed") {
        env.scratch().makeDirectory("assets/server");

        REQUIRE(index.rebuild(env.files()) == std::error_code{});

        CHECK(index.find(NESTED_SCENE_ID, record));
        CHECK(findListed(index, "project://assets/common/later.tescene") != nullptr);
        CHECK(index.getListingRevision() == revision + 1);
    }
}

TEST_CASE("rebuild lists and indexes a file written since the last build and drops one removed since", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexRebuild"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.writeScene("assets/client/menu.tescene", CLIENT_SCENE_ID);
    AssetIndex index = env.buildIndex();

    std::filesystem::remove(env.scratch().root() / "assets" / "client" / "menu.tescene");
    env.writeScene("assets/server/arena.tescene", SERVER_SCENE_ID);

    REQUIRE(index.rebuild(env.files()) == std::error_code{});

    AssetRecord record;
    CHECK(index.find(COMMON_SCENE_ID, record));
    CHECK_FALSE(index.find(CLIENT_SCENE_ID, record));
    REQUIRE(index.find(SERVER_SCENE_ID, record));
    CHECK(record.side == Side::Server);
    CHECK(findListed(index, "project://assets/client/menu.tescene") == nullptr);
    const ListedEntry* arena = findListed(index, "project://assets/server/arena.tescene");
    REQUIRE(arena != nullptr);
    CHECK(arena->kind == ListedKind::Asset);
    CHECK(arena->id == SERVER_SCENE_ID);
}

TEST_CASE("rebuild before any build succeeded is refused", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexRebuildUnbuilt"};
    AssetIndex index;
    const TechEngineTests::FatalAssertGuard guard;

    SECTION("on an index never built") {
        REQUIRE_THROWS_AS(index.rebuild(env.files()), TechEngineTests::AssertFired);
    }

    SECTION("after a first build that failed") {
        std::filesystem::remove_all(env.scratch().root() / "assets" / "server");
        REQUIRE(index.build(env.files(), INDEX_ASSETS_ROOT) == FileError::NotFound);
        REQUIRE_THROWS_AS(index.rebuild(env.files()), TechEngineTests::AssertFired);
    }
}

TEST_CASE("build warns about an engine file it cannot read and is quiet about any other file", "[editor][assets][listing][os]") {
    const IndexScratch env{"assetIndexQuietSkips"};
    env.writeScene("assets/common/level.tescene", COMMON_SCENE_ID);
    env.scratch().writeFile("assets/client/glTF/Sponza.gltf", "{}");
    env.scratch().writeFile("assets/client/glTF/Sponza.bin", "buffer");
    env.scratch().writeFile("assets/client/glTF/wall.jpg", "jpeg bytes");
    env.scratch().writeFile("assets/client/glTF/LICENSE.md", "license");
    env.scratch().writeFile("assets/client/image.hdr", "radiance bytes");
    env.scratch().writeFile("assets/client/shaders/GeometryShader.frag", "#version 460 core\nvoid main() {}\n");
    env.scratch().writeFile("assets/common/.gitkeep", "");
    writeBytes(env.scratch(), "assets/common/renamed.bin", assetBytes(SCENE_KIND, CLIENT_SCENE_ID, "scene payload"));
    env.scratch().writeFile("assets/common/short.temesh", "tiny");
    env.scratch().writeFile("assets/common/Upper.TEMATERIAL", "tiny");
    writeBytes(env.scratch(), "assets/common/wrong-magic.tetexture", assetBytes(BlobKind{0x12345678, 1}, ResourceId(0x6000000000000006), "x"));
    const TechEngineTests::LogCaptureGuard logs;

    const AssetIndex index = env.buildIndex();

    for (const std::string_view quiet:
         {"project://assets/client/glTF/Sponza.gltf",
          "project://assets/client/glTF/Sponza.bin",
          "project://assets/client/glTF/wall.jpg",
          "project://assets/client/glTF/LICENSE.md",
          "project://assets/client/image.hdr",
          "project://assets/client/shaders/GeometryShader.frag",
          "project://assets/common/.gitkeep"}) {
        CHECK_FALSE(loggedSkip(logs, quiet));
    }
    for (const std::string_view warned: {"project://assets/common/short.temesh", "project://assets/common/Upper.TEMATERIAL", "project://assets/common/wrong-magic.tetexture"}) {
        CHECK(loggedSkip(logs, warned));
    }

    AssetRecord renamed;
    REQUIRE(index.find(CLIENT_SCENE_ID, renamed));
    CHECK(renamed.path == "project://assets/common/renamed.bin");
    CHECK(index.getRecords().size() == 2);
}
