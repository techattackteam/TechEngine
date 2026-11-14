#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ModelTestFiles.hpp>
#include <assets/TextureTestFiles.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::ImportMode;
using TechEngine::ImportSettings;
using TechEngine::Material;
using TechEngine::Mesh;
using TechEngine::Model;
using TechEngine::ModelImportRequest;
using TechEngine::ModelImportResult;
using TechEngine::ModelNode;
using TechEngine::ProjectError;
using TechEngine::ResourceId;
using TechEngine::Texture;
using TechEngine::UpAxis;
using TechEngineTests::ScratchDirectory;

static constexpr std::uint64_t FIRST_TEST_ID = 0x5000;
static constexpr float TOLERANCE = 1.0e-5f;
static constexpr std::string_view FOLDER = "project://assets/client/Kit";
static constexpr std::string_view MODEL_PATH = "project://assets/client/Kit/Kit.temodel";
static constexpr std::string_view MERGED_PATH = "project://assets/client/Kit/Kit.temesh";
static constexpr std::string_view HULL_PATH = "project://assets/client/Kit/Meshes/Hull.temesh";
static constexpr std::string_view TIRE_PATH = "project://assets/client/Kit/Meshes/Tire.temesh";
static constexpr std::string_view PAINT_PATH = "project://assets/client/Kit/Materials/Paint.tematerial";
static constexpr std::string_view GLASS_PATH = "project://assets/client/Kit/Materials/Glass.tematerial";

static constexpr std::string_view HULL_THEN_TIRE_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                        R"("nodes":[{"name":"Front","mesh":0},{"name":"Back","mesh":1}],)"
                                                        R"("meshes":[{"name":"Hull","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                        R"({"name":"Tire","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

// The same meshes in the other order, in the file and in the walk, so assimp gives each the other's index.
static constexpr std::string_view TIRE_THEN_HULL_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                        R"("nodes":[{"name":"Front","mesh":0},{"name":"Back","mesh":1}],)"
                                                        R"("meshes":[{"name":"Tire","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                        R"({"name":"Hull","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

static constexpr std::string_view HULL_THEN_RIM_BODY = R"("scenes":[{"nodes":[0,1]}],)"
                                                       R"("nodes":[{"name":"Front","mesh":0},{"name":"Back","mesh":1}],)"
                                                       R"("meshes":[{"name":"Hull","primitives":[{"attributes":{"POSITION":1},"indices":0}]},)"
                                                       R"({"name":"Rim","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])";

// GLTF_TWO_MATERIALS_BODY with a third node, whose mesh Trim uses a new material, Chrome.
static constexpr std::string_view THREE_MATERIALS_BODY = R"("scenes":[{"nodes":[0,2]}],)"
                                                         R"("nodes":[{"name":"Window","mesh":0,"children":[1]},{"name":"Door","mesh":1,"translation":[0,0,10]},{"name":"Frame","mesh":2}],)"
                                                         R"("materials":[{"name":"Paint"},{"name":"Glass"},{"name":"Chrome"}],)"
                                                         R"("meshes":[{"name":"Pane","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":1}]},)"
                                                         R"({"name":"Board","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":0}]},)"
                                                         R"({"name":"Trim","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":2}]}])";

class ReimportFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    std::uint64_t nextId = FIRST_TEST_ID;

    explicit ReimportFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    void writeSource(const std::string& text) {
        scratch.writeFile("assets/client/Kit.gltf", text);
    }

    std::error_code run(const bool reimport, const ImportMode mode, const ImportSettings& settings, ModelImportResult& out) {
        ModelImportRequest request;
        request.sourcePath = "project://assets/client/Kit.gltf";
        request.side = TechEngine::Side::Client;
        request.settings = settings;
        request.mode = mode;
        const TechEngine::ModelImporter importer(files, [this] {
            return ResourceId{nextId++};
        });
        return reimport ? importer.reimportFile(request, out) : importer.importFile(request, out);
    }

    ModelImportResult importFile(const ImportMode mode = ImportMode::Model, const ImportSettings& settings = ImportSettings{}) {
        ModelImportResult result;
        REQUIRE(run(false, mode, settings, result) == std::error_code{});
        return result;
    }

    ModelImportResult reimportFile(const ImportMode mode = ImportMode::Model) {
        ModelImportResult result;
        REQUIRE(run(true, mode, ImportSettings{}, result) == std::error_code{});
        return result;
    }

    std::vector<std::byte> bytesOf(const std::string_view path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        return bytes;
    }

    Model readModel(const std::string_view path) const {
        Model model;
        REQUIRE(model.read(bytesOf(path)) == std::error_code{});
        return model;
    }

    Mesh readMesh(const std::string_view path) const {
        Mesh mesh;
        REQUIRE(mesh.read(bytesOf(path)) == std::error_code{});
        return mesh;
    }

    Material readMaterial(const std::string_view path) const {
        Material material;
        REQUIRE(material.read(bytesOf(path)) == std::error_code{});
        return material;
    }

    Texture readTexture(const std::string_view path) const {
        Texture texture;
        REQUIRE(texture.read(bytesOf(path)) == std::error_code{});
        return texture;
    }

    bool exists(const std::string_view relative) const {
        return std::filesystem::exists(scratch.root() / relative);
    }
};

static bool rewrote(const ModelImportResult& result, const ResourceId id) {
    return std::ranges::find(result.rewritten, id) != result.rewritten.end();
}

static const ModelNode& nodeNamed(const Model& model, const std::string_view name) {
    const auto found = std::ranges::find(model.nodes, name, &ModelNode::name);
    REQUIRE(found != model.nodes.end());
    return *found;
}

// One node and one mesh per material, each drawing gltfFile's triangle with its material.
static std::string materialsGltf(const std::size_t materialCount, const std::string_view materialsJson) {
    std::string sceneNodes;
    std::string nodes;
    std::string meshes;
    for (std::size_t i = 0; i < materialCount; i++) {
        const std::string separator = i == 0 ? "" : ",";
        const std::string index = std::to_string(i);
        sceneNodes += separator + index;
        nodes += separator + R"({"name":"Part)" + index + R"(","mesh":)" + index + "}";
        meshes += separator + R"({"name":"Part)" + index + R"(","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":)" + index + "}]}";
    }
    return TechEngineTests::gltfFile(R"("scenes":[{"nodes":[)" + sceneNodes + R"(]}],"nodes":[)" + nodes + R"(],"meshes":[)" + meshes + "]," + std::string(materialsJson));
}

TEST_CASE("re-importing an unchanged fixture keeps every id and rewrites everything but the materials", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportUnchanged"};
    fixture.writeSource(TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_MATERIALS_BODY));
    const ModelImportResult first = fixture.importFile();
    const Model model = fixture.readModel(MODEL_PATH);
    const std::vector<std::byte> paintBytes = fixture.bytesOf(PAINT_PATH);
    const std::vector<std::byte> glassBytes = fixture.bytesOf(GLASS_PATH);

    const ModelImportResult second = fixture.reimportFile();

    CHECK(second.records.empty());
    CHECK(second.folder == FOLDER);
    for (const AssetRecord& record: first.records) {
        if (record.typeTag == TechEngine::MATERIAL_TYPE_TAG) {
            CHECK_FALSE(rewrote(second, record.id));
            continue;
        }
        CHECK(rewrote(second, record.id));
    }
    CHECK(second.rewritten.size() == 3);

    const Model again = fixture.readModel(MODEL_PATH);
    CHECK(again.id == model.id);
    REQUIRE(again.nodes.size() == model.nodes.size());
    for (std::size_t i = 0; i < model.nodes.size(); i++) {
        CHECK(again.nodes[i].name == model.nodes[i].name);
        CHECK(again.nodes[i].mesh == model.nodes[i].mesh);
    }
    CHECK(fixture.readMesh("project://assets/client/Kit/Meshes/Pane.temesh").id == nodeNamed(model, "Kit").mesh);
    CHECK(fixture.readMesh("project://assets/client/Kit/Meshes/Board.temesh").id == nodeNamed(model, "Door").mesh);
    CHECK(fixture.bytesOf(PAINT_PATH) == paintBytes);
    CHECK(fixture.bytesOf(GLASS_PATH) == glassBytes);
}

TEST_CASE("meshes reordered in the source keep each id on its named mesh", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportReordered"};
    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    fixture.importFile();
    const Mesh hull = fixture.readMesh(HULL_PATH);
    const Mesh tire = fixture.readMesh(TIRE_PATH);
    REQUIRE(hull.importBlock.sourceName == "Hull");
    REQUIRE(tire.importBlock.sourceName == "Tire");

    fixture.writeSource(TechEngineTests::gltfFile(TIRE_THEN_HULL_BODY));
    fixture.reimportFile();

    const Mesh hullAgain = fixture.readMesh(HULL_PATH);
    const Mesh tireAgain = fixture.readMesh(TIRE_PATH);
    CHECK(hullAgain.importBlock.sourceIndex != hull.importBlock.sourceIndex);
    CHECK(hullAgain.importBlock.sourceName == "Hull");
    CHECK(hullAgain.id == hull.id);
    CHECK(tireAgain.importBlock.sourceName == "Tire");
    CHECK(tireAgain.id == tire.id);
    const Model model = fixture.readModel(MODEL_PATH);
    CHECK(nodeNamed(model, "Front").mesh == tire.id);
    CHECK(nodeNamed(model, "Back").mesh == hull.id);
}

TEST_CASE("a new mesh gets a fresh id and a removed mesh's file stays as it was", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportNewAndRemoved"};
    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    const ModelImportResult first = fixture.importFile();
    const Mesh hull = fixture.readMesh(HULL_PATH);
    const std::vector<std::byte> tireBytes = fixture.bytesOf(TIRE_PATH);

    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_RIM_BODY));
    const ModelImportResult second = fixture.reimportFile();

    const Mesh rim = fixture.readMesh("project://assets/client/Kit/Meshes/Rim.temesh");
    CHECK(fixture.readMesh(HULL_PATH).id == hull.id);
    for (const AssetRecord& record: first.records) {
        CHECK(rim.id != record.id);
    }
    REQUIRE(second.records.size() == 1);
    CHECK(second.records[0].id == rim.id);
    CHECK(second.records[0].typeTag == TechEngine::MESH_TYPE_TAG);
    CHECK(second.records[0].side == TechEngine::Side::Client);
    CHECK(second.records[0].path == "project://assets/client/Kit/Meshes/Rim.temesh");
    CHECK_FALSE(rewrote(second, rim.id));

    CHECK(fixture.bytesOf(TIRE_PATH) == tireBytes);
    const Model model = fixture.readModel(MODEL_PATH);
    CHECK(nodeNamed(model, "Back").mesh == rim.id);
}

TEST_CASE("a new mesh whose file name an unmatched old file holds gets its own file", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportNameClash"};
    fixture.writeSource(TechEngineTests::gltfFile(TechEngineTests::GLTF_CLASHING_NAMES_BODY));
    fixture.importFile();
    const std::vector<std::byte> first = fixture.bytesOf("project://assets/client/Kit/Meshes/Part_A.temesh");
    REQUIRE(fixture.readMesh("project://assets/client/Kit/Meshes/Part_A.temesh").importBlock.sourceName == "Part/A");

    fixture.writeSource(TechEngineTests::gltfFile(R"("scenes":[{"nodes":[0]}],"nodes":[{"name":"Only","mesh":0}],)"
                                                  R"("meshes":[{"name":"Part:A","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])"));
    const ModelImportResult second = fixture.reimportFile();

    CHECK(fixture.bytesOf("project://assets/client/Kit/Meshes/Part_A.temesh") == first);
    REQUIRE(second.records.size() == 1);
    CHECK(second.records[0].path == "project://assets/client/Kit/Meshes/Part_A_3.temesh");
}

TEST_CASE("a matched material's file is never rewritten, and a new material is written", "[editor][assets][reimport][materials]") {
    ReimportFixture fixture{"reimportMaterials"};
    fixture.writeSource(TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_MATERIALS_BODY));
    fixture.importFile();
    Material paint = fixture.readMaterial(PAINT_PATH);
    paint.baseColorFactor = TechEngine::Vec4(0.1f, 0.2f, 0.3f, 1.0f);
    std::vector<std::byte> edited;
    paint.write(edited);
    REQUIRE(fixture.files.write(PAINT_PATH, edited) == std::error_code{});
    const std::vector<std::byte> glassBytes = fixture.bytesOf(GLASS_PATH);
    const ResourceId glassId = fixture.readMaterial(GLASS_PATH).id;

    fixture.writeSource(TechEngineTests::gltfFile(THREE_MATERIALS_BODY));
    const ModelImportResult second = fixture.reimportFile();

    CHECK(fixture.bytesOf(PAINT_PATH) == edited);
    CHECK(fixture.bytesOf(GLASS_PATH) == glassBytes);
    CHECK_FALSE(rewrote(second, paint.id));
    CHECK_FALSE(rewrote(second, glassId));

    const Material chrome = fixture.readMaterial("project://assets/client/Kit/Materials/Chrome.tematerial");
    CHECK(chrome.importBlock.sourceName == "Chrome");
    const auto chromeRecord = std::ranges::find(second.records, chrome.id, &AssetRecord::id);
    REQUIRE(chromeRecord != second.records.end());
    CHECK(chromeRecord->typeTag == TechEngine::MATERIAL_TYPE_TAG);

    CHECK(fixture.readMesh("project://assets/client/Kit/Meshes/Board.temesh").slots.at(0).defaultMaterial == paint.id);
    CHECK(fixture.readMesh("project://assets/client/Kit/Meshes/Pane.temesh").slots.at(0).defaultMaterial == glassId);
    CHECK(fixture.readMesh("project://assets/client/Kit/Meshes/Trim.temesh").slots.at(0).defaultMaterial == chrome.id);
}

TEST_CASE("textures keep their ids by key name when the materials around them change", "[editor][assets][reimport][materials]") {
    ReimportFixture fixture{"reimportTextures"};
    fixture.scratch.writeFile("assets/client/Textures/wall.png", TechEngineTests::solidPng(4, 4, {200, 100, 50, 255}));
    fixture.writeSource(materialsGltf(
        1,
        R"("materials":[{"name":"Brick","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],)"
        R"("textures":[{"source":0}],"images":[{"uri":"Textures/wall.png"}])"));
    fixture.importFile();
    const std::string brickPath = "project://assets/client/Kit/Materials/Brick.tematerial";
    const std::string wallPath = "project://assets/client/Kit/Textures/wall_basecolor.tetexture";
    const Material brick = fixture.readMaterial(brickPath);
    const Texture wall = fixture.readTexture(wallPath);
    REQUIRE(brick.baseColorTexture.id == wall.id);

    fixture.scratch.writeFile("assets/client/Textures/glow.png", TechEngineTests::solidPng(4, 4, {10, 20, 30, 255}));
    fixture.writeSource(materialsGltf(
        2,
        R"("materials":[{"name":"Lamp","emissiveTexture":{"index":1}},)"
        R"({"name":"Brick","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],)"
        R"("textures":[{"source":0},{"source":1}],"images":[{"uri":"Textures/wall.png"},{"uri":"Textures/glow.png"}])"));
    const ModelImportResult second = fixture.reimportFile();

    const Texture wallAgain = fixture.readTexture(wallPath);
    CHECK(wallAgain.id == wall.id);
    CHECK(wallAgain.importBlock.sourceName == "Textures/wall.png#basecolor#repeat-linear");
    CHECK(rewrote(second, wall.id));
    CHECK(fixture.readMaterial(brickPath).id == brick.id);

    const Texture glow = fixture.readTexture("project://assets/client/Kit/Textures/glow_emissive.tetexture");
    CHECK(glow.id != wall.id);
    CHECK(fixture.readMaterial("project://assets/client/Kit/Materials/Lamp.tematerial").emissiveTexture.id == glow.id);
    CHECK(std::ranges::find(second.records, glow.id, &AssetRecord::id) != second.records.end());
}

TEST_CASE("a re-import takes its settings from the root asset's import block, not from the request", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportSettings"};
    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    const ImportSettings settings{0.01f, UpAxis::Y};
    fixture.importFile(ImportMode::Model, settings);

    fixture.reimportFile();

    CHECK(fixture.readModel(MODEL_PATH).importBlock.settings == settings);
    const Mesh hull = fixture.readMesh(HULL_PATH);
    REQUIRE(hull.positions.size() == 3);
    CHECK(hull.positions[1].x == Catch::Approx(0.04f).margin(TOLERANCE));
    CHECK(hull.positions[1].y == Catch::Approx(0.02f).margin(TOLERANCE));
    CHECK(hull.positions[1].z == Catch::Approx(0.03f).margin(TOLERANCE));
}

TEST_CASE("re-importing the other way switches the root asset and keeps the old root and the materials", "[editor][assets][reimport][merged]") {
    ReimportFixture fixture{"reimportSwitch"};
    fixture.writeSource(TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_MATERIALS_BODY));
    const ImportSettings settings{2.0f, UpAxis::Z};
    fixture.importFile(ImportMode::Model, settings);
    const std::vector<std::byte> modelBytes = fixture.bytesOf(MODEL_PATH);
    const std::vector<std::byte> paneBytes = fixture.bytesOf("project://assets/client/Kit/Meshes/Pane.temesh");
    const std::vector<std::byte> paintBytes = fixture.bytesOf(PAINT_PATH);
    const ResourceId paintId = fixture.readMaterial(PAINT_PATH).id;
    const ResourceId glassId = fixture.readMaterial(GLASS_PATH).id;
    const ResourceId modelId = fixture.readModel(MODEL_PATH).id;

    const ModelImportResult merged = fixture.reimportFile(ImportMode::MergedMesh);

    REQUIRE(merged.records.size() == 1);
    CHECK(merged.records[0].path == MERGED_PATH);
    CHECK(merged.records[0].typeTag == TechEngine::MESH_TYPE_TAG);
    const Mesh root = fixture.readMesh(MERGED_PATH);
    CHECK(root.id == merged.records[0].id);
    CHECK(root.id != modelId);
    CHECK(root.importBlock.settings == settings);
    REQUIRE(root.slots.size() == 2);
    CHECK(root.slots[0].defaultMaterial == paintId);
    CHECK(root.slots[1].defaultMaterial == glassId);
    CHECK(fixture.bytesOf(MODEL_PATH) == modelBytes);
    CHECK(fixture.bytesOf("project://assets/client/Kit/Meshes/Pane.temesh") == paneBytes);
    CHECK(fixture.bytesOf(PAINT_PATH) == paintBytes);
    CHECK(merged.rewritten.empty());

    SECTION("and back again, both roots keep their ids") {
        const ModelImportResult model = fixture.reimportFile(ImportMode::Model);

        CHECK(model.records.empty());
        CHECK(fixture.readModel(MODEL_PATH).id == modelId);
        CHECK(fixture.readMesh(MERGED_PATH).id == root.id);
        CHECK(rewrote(model, modelId));
        CHECK_FALSE(rewrote(model, root.id));
    }
}

TEST_CASE("a re-import with no earlier import is refused and writes nothing", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportMissing"};
    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    ModelImportResult result;

    SECTION("no folder") {
        CHECK(fixture.run(true, ImportMode::Model, ImportSettings{}, result) == ProjectError::ReimportTargetMissing);
        CHECK_FALSE(fixture.exists("assets/client/Kit"));
    }

    SECTION("a folder with no root asset") {
        fixture.scratch.makeDirectory("assets/client/Kit");
        CHECK(fixture.run(true, ImportMode::Model, ImportSettings{}, result) == ProjectError::ReimportRootMissing);
        CHECK(std::filesystem::is_empty(fixture.scratch.root() / "assets" / "client" / "Kit"));
    }

    SECTION("a plain import still refuses the folder an import wrote") {
        fixture.importFile();
        CHECK(fixture.run(false, ImportMode::Model, ImportSettings{}, result) == ProjectError::ImportFolderExists);
    }
}

TEST_CASE("a source imported inside another's folder keeps its files when the outer source is re-imported", "[editor][assets][reimport]") {
    ReimportFixture fixture{"reimportNestedSource"};
    fixture.writeSource(TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    fixture.importFile();
    const Mesh hull = fixture.readMesh(HULL_PATH);
    fixture.scratch.writeFile("assets/client/Kit/Inner.gltf", TechEngineTests::gltfFile(HULL_THEN_TIRE_BODY));
    ModelImportRequest inner;
    inner.sourcePath = "project://assets/client/Kit/Inner.gltf";
    inner.side = TechEngine::Side::Client;
    ModelImportResult innerResult;
    const TechEngine::ModelImporter importer(fixture.files, [&fixture] {
        return ResourceId{fixture.nextId++};
    });
    REQUIRE(importer.importFile(inner, innerResult) == std::error_code{});
    const std::string innerHullPath = "project://assets/client/Kit/Inner/Meshes/Hull.temesh";
    const std::vector<std::byte> innerHullBytes = fixture.bytesOf(innerHullPath);

    const ModelImportResult second = fixture.reimportFile();

    CHECK(fixture.bytesOf(innerHullPath) == innerHullBytes);
    CHECK(fixture.readMesh(HULL_PATH).id == hull.id);
    CHECK(rewrote(second, hull.id));
    CHECK(second.records.empty());
    for (const AssetRecord& record: innerResult.records) {
        CHECK_FALSE(rewrote(second, record.id));
    }
}
