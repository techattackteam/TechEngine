#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ModelTestFiles.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::ImportSettings;
using TechEngine::Mesh;
using TechEngine::Model;
using TechEngine::ModelImportRequest;
using TechEngine::ModelImportResult;
using TechEngine::ModelNode;
using TechEngine::ProjectError;
using TechEngine::Quat;
using TechEngine::ResourceId;
using TechEngine::UpAxis;
using TechEngine::Vec2;
using TechEngine::Vec3;
using TechEngine::Vec4;
using TechEngineTests::ScratchDirectory;

static constexpr std::uint64_t FIRST_TEST_ID = 0x1000;
static constexpr float TOLERANCE = 1.0e-5f;

class ModelImportFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    std::uint64_t nextId = FIRST_TEST_ID;

    explicit ModelImportFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    std::error_code importFile(const std::string_view fileName, const ImportSettings& settings, ModelImportResult& out) {
        ModelImportRequest request;
        request.sourcePath = "project://assets/client/" + std::string(fileName);
        request.side = TechEngine::Side::Client;
        request.settings = settings;
        const TechEngine::ModelImporter importer(files, [this] {
            return ResourceId{nextId++};
        });
        return importer.importFile(request, out);
    }

    Model readModel(const std::string& path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Model model;
        REQUIRE(model.read(bytes) == std::error_code{});
        return model;
    }

    Mesh readMesh(const std::string& path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Mesh mesh;
        REQUIRE(mesh.read(bytes) == std::error_code{});
        return mesh;
    }

    bool exists(const std::string_view relative) const {
        return std::filesystem::exists(scratch.root() / relative);
    }
};

static const ModelNode& nodeNamed(const Model& model, const std::string_view name) {
    const auto found = std::ranges::find(model.nodes, name, &ModelNode::name);
    REQUIRE(found != model.nodes.end());
    return *found;
}

// assimp's glTF reader lists meshes in the order the nodes first name them, not the file's order.
static const AssetRecord& recordNamed(const ModelImportResult& result, const std::string_view fileName) {
    const auto found = std::ranges::find_if(result.records, [fileName](const AssetRecord& record) {
        return std::filesystem::path(record.path).filename().string() == fileName;
    });
    REQUIRE(found != result.records.end());
    return *found;
}

static std::vector<AssetRecord> meshRecords(const ModelImportResult& result) {
    std::vector<AssetRecord> meshes;
    std::ranges::copy_if(result.records, std::back_inserter(meshes), [](const AssetRecord& record) {
        return record.typeTag == TechEngine::MESH_TYPE_TAG;
    });
    return meshes;
}

static std::uint32_t indexOf(const Model& model, const ModelNode& node) {
    return static_cast<std::uint32_t>(&node - model.nodes.data());
}

static void checkVec3(const Vec3 actual, const Vec3 expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(TOLERANCE));
    CHECK(actual.y == Catch::Approx(expected.y).margin(TOLERANCE));
    CHECK(actual.z == Catch::Approx(expected.z).margin(TOLERANCE));
}

// q and -q are the same rotation, and assimp's decomposition may return either.
static void checkRotation(const Quat actual, const Quat expected) {
    CHECK(std::abs(glm::dot(actual, expected)) == Catch::Approx(1.0f).margin(TOLERANCE));
}

static void checkIdentity(const TechEngine::TransformValues& local) {
    checkVec3(local.position, Vec3(0.0f));
    checkRotation(local.rotation, Quat(1.0f, 0.0f, 0.0f, 0.0f));
    checkVec3(local.scale, Vec3(1.0f));
}

static std::ptrdiff_t entryCount(const std::filesystem::path& directory) {
    return std::distance(std::filesystem::directory_iterator{directory}, std::filesystem::directory_iterator{});
}

static Vec3 turnedUpright(const Vec3 value) {
    return Vec3(value.x, value.z, -value.y);
}

static void checkVec2(const Vec2 actual, const Vec2 expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(TOLERANCE));
    CHECK(actual.y == Catch::Approx(expected.y).margin(TOLERANCE));
}

static void checkUnitTangent(const Vec4 tangent, const Vec3 normal) {
    CHECK(glm::length(Vec3(tangent)) == Catch::Approx(1.0f).margin(TOLERANCE));
    CHECK(glm::dot(Vec3(tangent), normal) == Catch::Approx(0.0f).margin(TOLERANCE));
    CHECK((tangent.w == 1.0f || tangent.w == -1.0f));
}

// The direction in which UV0's V grows across one triangle, solved from its positions and UVs.
static Vec3 uvBitangent(const Mesh& mesh, const std::size_t firstIndex) {
    const std::uint32_t a = mesh.indices[firstIndex];
    const std::uint32_t b = mesh.indices[firstIndex + 1];
    const std::uint32_t c = mesh.indices[firstIndex + 2];
    const Vec3 edge1 = mesh.positions[b] - mesh.positions[a];
    const Vec3 edge2 = mesh.positions[c] - mesh.positions[a];
    const Vec2 uvEdge1 = mesh.uv0[b] - mesh.uv0[a];
    const Vec2 uvEdge2 = mesh.uv0[c] - mesh.uv0[a];
    const float determinant = uvEdge1.x * uvEdge2.y - uvEdge2.x * uvEdge1.y;
    REQUIRE(std::abs(determinant) > TOLERANCE);
    return (edge2 * uvEdge1.x - edge1 * uvEdge2.x) / determinant;
}

// w is right when cross(n, t.xyz) * w points the way V grows in the stored UV0, at every corner of every triangle.
static void checkBitangentsFollowUv(const Mesh& mesh) {
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    REQUIRE(mesh.tangents.size() == mesh.positions.size());
    REQUIRE(mesh.uv0.size() == mesh.positions.size());
    for (std::size_t first = 0; first < mesh.indices.size(); first += 3) {
        const Vec3 expected = uvBitangent(mesh, first);
        for (std::size_t corner = 0; corner < 3; corner++) {
            const std::uint32_t vertex = mesh.indices[first + corner];
            const Vec4 tangent = mesh.tangents[vertex];
            const Vec3 built = glm::cross(mesh.normals[vertex], Vec3(tangent)) * tangent.w;
            CHECK(glm::dot(built, expected) > 0.0f);
        }
    }
}

static std::string withExtensionsUsed(const std::string_view extensionsArray, const std::string_view body) {
    return R"("extensionsUsed":)" + std::string(extensionsArray) + "," + std::string(body);
}

TEST_CASE("the import folder is the source path without its extension", "[editor][assets][import]") {
    CHECK(TechEngine::ModelImporter::importFolder("project://assets/client/Sponza.gltf") == "project://assets/client/Sponza");
    CHECK(TechEngine::ModelImporter::importFolder("project://assets/client/Old.Town.fbx") == "project://assets/client/Old.Town");
    CHECK(TechEngine::ModelImporter::importFolder("project://assets/v1.2/Crate.obj") == "project://assets/v1.2/Crate");
    CHECK(TechEngine::ModelImporter::importFolder("project://assets/v1.2/Crate") == "project://assets/v1.2/Crate");
    CHECK(TechEngine::ModelImporter::importFolder("project://assets/client/.hidden") == "project://assets/client/.hidden");
}

TEST_CASE("a glTF file imports with its node and mesh counts", "[editor][assets][import]") {
    ModelImportFixture fixture{"importPairCounts"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Pair.gltf", ImportSettings{}, result) == std::error_code{});

    CHECK(result.folder == "project://assets/client/Pair");
    CHECK(result.nodeCount == 4);
    REQUIRE(result.records.size() == 4);

    SECTION("the records name the model first, then each mesh, then the material, on the source's side") {
        CHECK(result.records[0].typeTag == TechEngine::MODEL_TYPE_TAG);
        CHECK(result.records[0].path == "project://assets/client/Pair/Pair.temodel");
        CHECK(result.records[1].typeTag == TechEngine::MESH_TYPE_TAG);
        CHECK(result.records[2].typeTag == TechEngine::MESH_TYPE_TAG);
        CHECK(result.records[3].typeTag == TechEngine::MATERIAL_TYPE_TAG);
        CHECK(recordNamed(result, "Hull.temesh").path == "project://assets/client/Pair/Meshes/Hull.temesh");
        CHECK(recordNamed(result, "Tire.temesh").path == "project://assets/client/Pair/Meshes/Tire.temesh");
        for (const AssetRecord& record: result.records) {
            CHECK(record.side == TechEngine::Side::Client);
            CHECK(record.id.value() >= FIRST_TEST_ID);
            CHECK(record.id.value() < fixture.nextId);
        }
        CHECK(result.records[0].id != result.records[1].id);
        CHECK(result.records[0].id != result.records[2].id);
        CHECK(result.records[1].id != result.records[2].id);
    }

    SECTION("the folder holds the model, a Meshes folder with one file per mesh and a Materials folder") {
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair") == 3);
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair" / "Meshes") == 2);
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client") == 2);
    }

    SECTION("each file carries the id of its record") {
        CHECK(fixture.readModel(result.records[0].path).id == result.records[0].id);
        CHECK(fixture.readMesh(result.records[1].path).id == result.records[1].id);
        CHECK(fixture.readMesh(result.records[2].path).id == result.records[2].id);
    }

    SECTION("the model has one root named after the file above the source's nodes") {
        const Model model = fixture.readModel(result.records[0].path);
        REQUIRE(model.nodes.size() == 4);
        CHECK(model.nodes[0].name == "Pair");
        CHECK(model.nodes[0].parent == TechEngine::MODEL_NO_PARENT);
        CHECK_FALSE(model.nodes[0].mesh.valid());
        checkIdentity(model.nodes[0].local);

        const ModelNode& body = nodeNamed(model, "Body");
        const ModelNode& wheel = nodeNamed(model, "Wheel");
        const ModelNode& empty = nodeNamed(model, "Empty");
        CHECK(body.parent == 0);
        CHECK(wheel.parent == indexOf(model, body));
        CHECK(empty.parent == 0);
        CHECK(indexOf(model, body) < indexOf(model, empty));
        CHECK(body.mesh == recordNamed(result, "Hull.temesh").id);
        CHECK(wheel.mesh == recordNamed(result, "Tire.temesh").id);
        CHECK_FALSE(empty.mesh.valid());
    }

    SECTION("with no scale and Y up, nodes and vertices keep the source's values") {
        const Model model = fixture.readModel(result.records[0].path);
        checkVec3(nodeNamed(model, "Body").local.position, Vec3(0.0f, 0.0f, 5.0f));
        checkVec3(nodeNamed(model, "Body").local.scale, Vec3(1.0f));
        checkRotation(nodeNamed(model, "Wheel").local.rotation, Quat(0.70710678f, 0.0f, 0.0f, 0.70710678f));
        checkVec3(nodeNamed(model, "Wheel").local.scale, Vec3(1.0f, 2.0f, 3.0f));
        checkIdentity(nodeNamed(model, "Empty").local);

        const Mesh hull = fixture.readMesh(recordNamed(result, "Hull.temesh").path);
        REQUIRE(hull.positions.size() == 3);
        checkVec3(hull.positions[0], Vec3(1.0f, 2.0f, 3.0f));
        checkVec3(hull.positions[1], Vec3(4.0f, 2.0f, 3.0f));
        checkVec3(hull.positions[2], Vec3(1.0f, 5.0f, 3.0f));
        CHECK(hull.indices == std::vector<std::uint32_t>{0, 1, 2});
    }

    SECTION("the model's import block names the source, an empty key and the settings") {
        const Model model = fixture.readModel(result.records[0].path);
        CHECK(model.importBlock.sourcePath == "assets/client/Pair.gltf");
        CHECK(model.importBlock.sourceIndex == 0);
        CHECK(model.importBlock.sourceName.empty());
        CHECK(model.importBlock.settings == ImportSettings{});
    }

    SECTION("each mesh keys on its source mesh and has one slot over all its indices") {
        const Mesh hull = fixture.readMesh(recordNamed(result, "Hull.temesh").path);
        const Mesh tire = fixture.readMesh(recordNamed(result, "Tire.temesh").path);
        CHECK(hull.importBlock.sourcePath == "assets/client/Pair.gltf");
        CHECK(hull.importBlock.sourceName == "Hull");
        CHECK(tire.importBlock.sourcePath == "assets/client/Pair.gltf");
        CHECK(tire.importBlock.sourceName == "Tire");
        CHECK(hull.importBlock.sourceIndex != tire.importBlock.sourceIndex);
        CHECK(hull.importBlock.sourceIndex < 2);
        CHECK(tire.importBlock.sourceIndex < 2);
        for (const Mesh* mesh: {&hull, &tire}) {
            CHECK(mesh->importBlock.settings == ImportSettings{});
            REQUIRE(mesh->slots.size() == 1);
            CHECK_FALSE(mesh->slots[0].name.empty());
            CHECK(mesh->slots[0].firstIndex == 0);
            CHECK(mesh->slots[0].indexCount == mesh->indices.size());
            CHECK(mesh->slots[0].defaultMaterial == result.records[3].id);
        }
    }

    SECTION("a mesh with positions only gets flat normals, and no tangents or UV0") {
        for (const AssetRecord& record: meshRecords(result)) {
            const Mesh mesh = fixture.readMesh(record.path);
            REQUIRE(mesh.normals.size() == mesh.positions.size());
            for (const Vec3 normal: mesh.normals) {
                checkVec3(normal, Vec3(0.0f, 0.0f, 1.0f));
            }
            CHECK(mesh.tangents.empty());
            CHECK(mesh.uv0.empty());
        }
    }
}

TEST_CASE("a scale of 0.01 and a Z-up axis are baked into the vertices and the nodes, and the root stays at identity", "[editor][assets][import]") {
    ModelImportFixture fixture{"importPairBaked"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    const ImportSettings settings{0.01f, UpAxis::Z};

    ModelImportResult result;
    REQUIRE(fixture.importFile("Pair.gltf", settings, result) == std::error_code{});
    REQUIRE(result.records.size() == 4);
    const Model model = fixture.readModel(result.records[0].path);

    SECTION("the root is at identity") {
        checkIdentity(model.nodes[0].local);
    }

    SECTION("a vertex at (x, y, z) lands at (x, z, -y) times the scale") {
        for (const AssetRecord& record: meshRecords(result)) {
            const Mesh mesh = fixture.readMesh(record.path);
            REQUIRE(mesh.positions.size() == 3);
            checkVec3(mesh.positions[0], Vec3(0.01f, 0.03f, -0.02f));
            checkVec3(mesh.positions[1], Vec3(0.04f, 0.03f, -0.02f));
            checkVec3(mesh.positions[2], Vec3(0.01f, 0.03f, -0.05f));
            CHECK(mesh.indices == std::vector<std::uint32_t>{0, 1, 2});
        }
    }

    SECTION("a generated normal is turned with the mesh and is not scaled") {
        for (const AssetRecord& record: meshRecords(result)) {
            const Mesh mesh = fixture.readMesh(record.path);
            REQUIRE(mesh.normals.size() == mesh.positions.size());
            for (const Vec3 normal: mesh.normals) {
                checkVec3(normal, Vec3(0.0f, 1.0f, 0.0f));
            }
        }
    }

    SECTION("only the model's import block carries the settings; each mesh keeps the defaults") {
        CHECK(model.importBlock.settings == settings);
        for (const AssetRecord& record: meshRecords(result)) {
            CHECK(fixture.readMesh(record.path).importBlock.settings == ImportSettings{});
        }
    }

    SECTION("a node's translation is turned and scaled") {
        checkVec3(nodeNamed(model, "Body").local.position, Vec3(0.0f, 0.05f, 0.0f));
        checkVec3(nodeNamed(model, "Wheel").local.position, Vec3(0.0f));
    }

    SECTION("a turn about the source's up axis becomes the same turn about Y") {
        checkRotation(nodeNamed(model, "Wheel").local.rotation, Quat(0.70710678f, 0.0f, 0.70710678f, 0.0f));
        checkRotation(nodeNamed(model, "Body").local.rotation, Quat(1.0f, 0.0f, 0.0f, 0.0f));
    }

    SECTION("a node's scale follows its axes and is not multiplied by the import scale") {
        checkVec3(nodeNamed(model, "Wheel").local.scale, Vec3(1.0f, 3.0f, 2.0f));
        checkVec3(nodeNamed(model, "Body").local.scale, Vec3(1.0f));
    }

    SECTION("a vertex of the wheel reaches the same world point as the source's, turned and scaled") {
        const ModelNode& body = nodeNamed(model, "Body");
        const ModelNode& wheel = nodeNamed(model, "Wheel");
        const Mesh tire = fixture.readMesh(recordNamed(result, "Tire.temesh").path);
        const TechEngine::TransformValues world = model.nodes[0].local * body.local * wheel.local;
        const Vec3 baked = world.position + world.rotation * (wheel.local.scale * tire.positions[1]);

        // In the source: scale (4, 2, 3) by (1, 2, 3), turn a quarter about Z, then move 5 along Z.
        const Vec3 source = Vec3(-4.0f, 4.0f, 14.0f);
        checkVec3(baked, Vec3(source.x, source.z, -source.y) * 0.01f);
    }

    SECTION("the import block keeps the settings for a later re-import") {
        CHECK(model.importBlock.settings == settings);
    }
}

TEST_CASE("a scale alone multiplies vertices and translations and turns nothing", "[editor][assets][import]") {
    ModelImportFixture fixture{"importPairScaled"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Pair.gltf", ImportSettings{2.0f, UpAxis::Y}, result) == std::error_code{});
    REQUIRE(result.records.size() == 4);
    const Model model = fixture.readModel(result.records[0].path);
    const Mesh hull = fixture.readMesh(recordNamed(result, "Hull.temesh").path);

    REQUIRE(hull.positions.size() == 3);
    checkVec3(hull.positions[1], Vec3(8.0f, 4.0f, 6.0f));
    checkVec3(nodeNamed(model, "Body").local.position, Vec3(0.0f, 0.0f, 10.0f));
    checkRotation(nodeNamed(model, "Wheel").local.rotation, Quat(0.70710678f, 0.0f, 0.0f, 0.70710678f));
    checkVec3(nodeNamed(model, "Wheel").local.scale, Vec3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("importing into a folder that exists is refused and writes nothing", "[editor][assets][import]") {
    ModelImportFixture fixture{"importFolderExists"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    SECTION("an empty folder") {
        fixture.scratch.makeDirectory("assets/client/Pair");

        ModelImportResult result;
        CHECK(fixture.importFile("Pair.gltf", ImportSettings{}, result) == ProjectError::ImportFolderExists);

        CHECK(result.records.empty());
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair") == 0);
        CHECK(fixture.nextId == FIRST_TEST_ID);
    }

    SECTION("a file with the folder's name") {
        fixture.scratch.writeFile("assets/client/Pair", "not a folder");

        ModelImportResult result;
        CHECK(fixture.importFile("Pair.gltf", ImportSettings{}, result) == ProjectError::ImportFolderExists);

        CHECK(std::filesystem::is_regular_file(fixture.scratch.root() / "assets" / "client" / "Pair"));
    }

    SECTION("the folder of an earlier import, whose files stay as they were") {
        ModelImportResult first;
        REQUIRE(fixture.importFile("Pair.gltf", ImportSettings{}, first) == std::error_code{});
        std::vector<std::byte> modelBefore;
        REQUIRE(fixture.files.read(first.records[0].path, modelBefore) == std::error_code{});
        std::vector<std::byte> meshBefore;
        REQUIRE(fixture.files.read(first.records[1].path, meshBefore) == std::error_code{});

        ModelImportResult second;
        CHECK(fixture.importFile("Pair.gltf", ImportSettings{0.5f, UpAxis::Z}, second) == ProjectError::ImportFolderExists);

        CHECK(second.records.empty());
        std::vector<std::byte> modelAfter;
        REQUIRE(fixture.files.read(first.records[0].path, modelAfter) == std::error_code{});
        std::vector<std::byte> meshAfter;
        REQUIRE(fixture.files.read(first.records[1].path, meshAfter) == std::error_code{});
        CHECK(modelAfter == modelBefore);
        CHECK(meshAfter == meshBefore);
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair" / "Meshes") == 2);
    }
}

TEST_CASE("a source that cannot be imported leaves no folder behind", "[editor][assets][import]") {
    ModelImportFixture fixture{"importFailures"};
    ModelImportResult result;

    SECTION("a missing file") {
        CHECK(fixture.importFile("Missing.gltf", ImportSettings{}, result) == TechEngine::FileError::NotFound);
        CHECK_FALSE(fixture.exists("assets/client/Missing"));
    }

    SECTION("a file assimp cannot parse") {
        fixture.scratch.writeFile("assets/client/Broken.gltf", "{ this is not a glTF file");
        CHECK(fixture.importFile("Broken.gltf", ImportSettings{}, result) == ProjectError::SourceUnreadable);
        CHECK_FALSE(fixture.exists("assets/client/Broken"));
    }

    SECTION("a format assimp was built without") {
        fixture.scratch.writeFile("assets/client/Shape.stl", "solid shape\nendsolid shape\n");
        CHECK(fixture.importFile("Shape.stl", ImportSettings{}, result) == ProjectError::SourceUnreadable);
        CHECK_FALSE(fixture.exists("assets/client/Shape"));
    }

    SECTION("a file with points and no triangles") {
        fixture.scratch.writeFile("assets/client/Dots.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_POINTS_ONLY_BODY));
        CHECK(fixture.importFile("Dots.gltf", ImportSettings{}, result) == ProjectError::SourceHasNoMeshes);
        CHECK_FALSE(fixture.exists("assets/client/Dots"));
    }

    SECTION("a path that is not a virtual path") {
        ModelImportRequest request;
        request.sourcePath = "assets/client/Pair.gltf";
        const TechEngine::ModelImporter importer(fixture.files, [] {
            return ResourceId{FIRST_TEST_ID};
        });
        const std::error_code importError = importer.importFile(request, result);
        CHECK(importError == TechEngine::FileError::InvalidPath);
    }

    CHECK(result.records.empty());
    CHECK(result.folder.empty());
    CHECK(entryCount(fixture.scratch.root() / "assets" / "client") <= 1);
}

TEST_CASE("a mesh that is not made of triangles is skipped, and its node carries no mesh", "[editor][assets][import]") {
    ModelImportFixture fixture{"importSkipsPoints"};
    fixture.scratch.writeFile("assets/client/Mixed.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_POINTS_AND_TRIANGLES_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Mixed.gltf", ImportSettings{}, result) == std::error_code{});

    REQUIRE(result.records.size() == 3);
    CHECK(result.records[1].path == "project://assets/client/Mixed/Meshes/Shell.temesh");
    CHECK(result.records[1].typeTag == TechEngine::MESH_TYPE_TAG);
    CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Mixed" / "Meshes") == 1);

    const Model model = fixture.readModel(result.records[0].path);
    REQUIRE(model.nodes.size() == 3);
    CHECK_FALSE(nodeNamed(model, "Dots").mesh.valid());
    CHECK(nodeNamed(model, "Solid").mesh == result.records[1].id);
}

TEST_CASE("a mesh used by two nodes is written once and both nodes name it", "[editor][assets][import]") {
    ModelImportFixture fixture{"importSharedMesh"};
    fixture.scratch.writeFile("assets/client/Shared.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_SHARED_MESH_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Shared.gltf", ImportSettings{}, result) == std::error_code{});

    REQUIRE(result.records.size() == 3);
    const Model model = fixture.readModel(result.records[0].path);
    REQUIRE(model.nodes.size() == 3);
    CHECK(nodeNamed(model, "Left").mesh == result.records[1].id);
    CHECK(nodeNamed(model, "Right").mesh == result.records[1].id);
    CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Shared" / "Meshes") == 1);
}

TEST_CASE("a node with several meshes gets one child per mesh and carries none itself", "[editor][assets][import]") {
    ModelImportFixture fixture{"importTwoPrimitives"};
    fixture.scratch.writeFile("assets/client/Vehicle.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_PRIMITIVES_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Vehicle.gltf", ImportSettings{}, result) == std::error_code{});

    REQUIRE(result.records.size() == 4);
    const Model model = fixture.readModel(result.records[0].path);
    REQUIRE(model.nodes.size() == 3);

    SECTION("the source's only top-level node is the root: it takes the file's name and keeps its transform") {
        CHECK(model.nodes[0].name == "Vehicle");
        CHECK(model.nodes[0].parent == TechEngine::MODEL_NO_PARENT);
        checkVec3(model.nodes[0].local.position, Vec3(1.0f, 0.0f, 0.0f));
        CHECK_FALSE(model.nodes[0].mesh.valid());
    }

    SECTION("each child sits at identity under the node, named after its mesh") {
        for (std::size_t i = 1; i < model.nodes.size(); i++) {
            CHECK(model.nodes[i].parent == 0);
            checkIdentity(model.nodes[i].local);
            CHECK(model.nodes[i].mesh == result.records[i].id);
            const std::string fileName = std::filesystem::path(result.records[i].path).stem().string();
            CHECK(model.nodes[i].name == fileName);
        }
        CHECK(model.nodes[1].name != model.nodes[2].name);
        CHECK(model.nodes[1].mesh != model.nodes[2].mesh);
    }
}

TEST_CASE("mesh names that clash as file names each get their own file", "[editor][assets][import]") {
    ModelImportFixture fixture{"importClashingNames"};
    fixture.scratch.writeFile("assets/client/Kit.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_CLASHING_NAMES_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Kit.gltf", ImportSettings{}, result) == std::error_code{});

    REQUIRE(result.records.size() == 5);
    CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Kit" / "Meshes") == 3);
    std::vector<std::string> fileNames;
    for (const AssetRecord& record: meshRecords(result)) {
        std::string fileName = std::filesystem::path(record.path).filename().string();
        std::ranges::transform(fileName, fileName.begin(), [](const char character) {
            return character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a') : character;
        });
        fileNames.push_back(std::move(fileName));
    }
    std::ranges::sort(fileNames);
    CHECK(fileNames == std::vector<std::string>{"part_a.temesh", "part_a_1.temesh", "part_a_2.temesh"});

    const Model model = fixture.readModel(result.records[0].path);
    std::vector<ResourceId> nodeMeshes{nodeNamed(model, "First").mesh, nodeNamed(model, "Second").mesh, nodeNamed(model, "Third").mesh};
    std::vector<ResourceId> fileMeshes;
    for (const AssetRecord& record: meshRecords(result)) {
        fileMeshes.push_back(fixture.readMesh(record.path).id);
    }
    std::ranges::sort(nodeMeshes);
    std::ranges::sort(fileMeshes);
    CHECK(nodeMeshes == fileMeshes);
    CHECK(std::ranges::adjacent_find(nodeMeshes) == nodeMeshes.end());
}

TEST_CASE("an OBJ file imports, with its quad split into two triangles", "[editor][assets][import]") {
    ModelImportFixture fixture{"importObjQuad"};
    fixture.scratch.writeFile("assets/client/Quad.obj", TechEngineTests::OBJ_QUAD);

    ModelImportResult result;
    REQUIRE(fixture.importFile("Quad.obj", ImportSettings{}, result) == std::error_code{});

    REQUIRE(result.records.size() == 3);
    const Mesh mesh = fixture.readMesh(result.records[1].path);
    CHECK(mesh.positions.size() == 4);
    CHECK(mesh.indices.size() == 6);

    const Model model = fixture.readModel(result.records[0].path);
    CHECK(model.nodes[0].name == "Quad");
    CHECK(model.importBlock.sourcePath == "assets/client/Quad.obj");
    const auto holder = std::ranges::find(model.nodes, mesh.id, &ModelNode::mesh);
    CHECK(holder != model.nodes.end());
}

TEST_CASE("a glTF fixture with tangents keeps them, with w turned to the stored UVs", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importKeepsTangents"};
    fixture.scratch.writeFile("assets/client/Panel.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITH_TANGENTS_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Panel.gltf", ImportSettings{}, result) == std::error_code{});
    REQUIRE(result.records.size() == 3);
    const Mesh mesh = fixture.readMesh(result.records[1].path);
    REQUIRE(mesh.positions.size() == 4);
    CHECK(mesh.indices.size() == 6);

    SECTION("the file's normals are kept, not replaced by flat ones") {
        REQUIRE(mesh.normals.size() == 4);
        for (const Vec3 normal: mesh.normals) {
            checkVec3(normal, TechEngineTests::QUAD_NORMAL);
        }
    }

    SECTION("UV0 holds the file's UVs with V flipped, as assimp gives them") {
        REQUIRE(mesh.uv0.size() == 4);
        for (std::size_t i = 0; i < mesh.positions.size(); i++) {
            const Vec3 position = mesh.positions[i];
            checkVec2(mesh.uv0[i], Vec2((position.x - 1.0f) / 3.0f, 1.0f - (position.y - 2.0f) / 3.0f));
        }
    }

    SECTION("each tangent keeps its direction, and w is the negation of glTF's, because V was flipped") {
        REQUIRE(mesh.tangents.size() == 4);
        for (const Vec4 tangent: mesh.tangents) {
            checkVec3(Vec3(tangent), Vec3(TechEngineTests::QUAD_TANGENT));
            CHECK(tangent.w == -TechEngineTests::QUAD_TANGENT.w);
        }
    }

    SECTION("the bitangent the tangents build follows V in the stored UVs") {
        checkBitangentsFollowUv(mesh);
    }
}

TEST_CASE("a glTF fixture without tangents gets unit MikkTSpace tangents with w of plus or minus one", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importMakesTangents"};
    fixture.scratch.writeFile("assets/client/Panel.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITHOUT_TANGENTS_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Panel.gltf", ImportSettings{}, result) == std::error_code{});
    REQUIRE(result.records.size() == 3);
    const Mesh mesh = fixture.readMesh(result.records[1].path);

    SECTION("every vertex has a unit tangent, perpendicular to its normal") {
        REQUIRE(mesh.tangents.size() == mesh.positions.size());
        REQUIRE(mesh.normals.size() == mesh.positions.size());
        for (std::size_t i = 0; i < mesh.tangents.size(); i++) {
            checkUnitTangent(mesh.tangents[i], mesh.normals[i]);
        }
    }

    SECTION("the mesh is re-indexed: the two shared corners are one vertex again") {
        CHECK(mesh.positions.size() == 4);
        CHECK(mesh.indices.size() == 6);
        REQUIRE(mesh.slots.size() == 1);
        CHECK(mesh.slots[0].firstIndex == 0);
        CHECK(mesh.slots[0].indexCount == 6);
        for (const std::uint32_t index: mesh.indices) {
            CHECK(index < mesh.positions.size());
        }
    }

    SECTION("the bitangent the tangents build follows V in the stored UVs") {
        checkBitangentsFollowUv(mesh);
    }
}

TEST_CASE("a generated tangent and a file tangent agree on the same quad", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importTangentsAgree"};
    fixture.scratch.writeFile("assets/client/Filed.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITH_TANGENTS_BODY));
    fixture.scratch.writeFile("assets/client/Made.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITHOUT_TANGENTS_BODY));

    ModelImportResult filedResult;
    REQUIRE(fixture.importFile("Filed.gltf", ImportSettings{}, filedResult) == std::error_code{});
    ModelImportResult madeResult;
    REQUIRE(fixture.importFile("Made.gltf", ImportSettings{}, madeResult) == std::error_code{});
    const Mesh filed = fixture.readMesh(filedResult.records[1].path);
    const Mesh made = fixture.readMesh(madeResult.records[1].path);
    REQUIRE(filed.tangents.size() == filed.positions.size());
    REQUIRE(made.tangents.size() == made.positions.size());

    for (std::size_t i = 0; i < made.positions.size(); i++) {
        const auto match = std::ranges::find_if(filed.positions, [&made, i](const Vec3 position) {
            return glm::length(position - made.positions[i]) < TOLERANCE;
        });
        REQUIRE(match != filed.positions.end());
        const Vec4 filedTangent = filed.tangents[static_cast<std::size_t>(match - filed.positions.begin())];
        checkVec3(Vec3(made.tangents[i]), Vec3(filedTangent));
        CHECK(made.tangents[i].w == filedTangent.w);
    }
}

TEST_CASE("a mesh with normals and no UV0 gets no tangents", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importNoUvNoTangents"};
    fixture.scratch.writeFile("assets/client/Panel.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITHOUT_UVS_BODY));

    ModelImportResult result;
    REQUIRE(fixture.importFile("Panel.gltf", ImportSettings{}, result) == std::error_code{});
    REQUIRE(result.records.size() == 3);
    const Mesh mesh = fixture.readMesh(result.records[1].path);

    CHECK(mesh.normals.size() == mesh.positions.size());
    CHECK(mesh.uv0.empty());
    CHECK(mesh.tangents.empty());
}

TEST_CASE("after a Z-up bake, cross(n, t.xyz) * w still matches the baked bitangent", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importTangentsBaked"};
    const ImportSettings settings{0.01f, UpAxis::Z};

    SECTION("the file's own tangents") {
        fixture.scratch.writeFile("assets/client/Panel.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITH_TANGENTS_BODY));
        ModelImportResult result;
        REQUIRE(fixture.importFile("Panel.gltf", settings, result) == std::error_code{});
        const Mesh mesh = fixture.readMesh(result.records[1].path);
        REQUIRE(mesh.normals.size() == 4);
        REQUIRE(mesh.tangents.size() == 4);

        // assimp's bitangent is glTF's, cross(n, t) * w; against the flipped V that the mesh stores, it points backwards.
        const Vec3 sourceNormal = TechEngineTests::QUAD_NORMAL;
        const Vec3 sourceTangent = Vec3(TechEngineTests::QUAD_TANGENT);
        const Vec3 storedBitangent = -glm::cross(sourceNormal, sourceTangent) * TechEngineTests::QUAD_TANGENT.w;

        for (std::size_t i = 0; i < 4; i++) {
            checkVec3(mesh.normals[i], turnedUpright(sourceNormal));
            CHECK(glm::length(mesh.normals[i]) == Catch::Approx(1.0f).margin(TOLERANCE));
            checkVec3(Vec3(mesh.tangents[i]), turnedUpright(sourceTangent));
            const Vec3 built = glm::cross(mesh.normals[i], Vec3(mesh.tangents[i])) * mesh.tangents[i].w;
            checkVec3(built, turnedUpright(storedBitangent));
        }
        checkBitangentsFollowUv(mesh);
    }

    SECTION("MikkTSpace's tangents, made after the bake") {
        fixture.scratch.writeFile("assets/client/Panel.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_WITHOUT_TANGENTS_BODY));
        ModelImportResult result;
        REQUIRE(fixture.importFile("Panel.gltf", settings, result) == std::error_code{});
        const Mesh mesh = fixture.readMesh(result.records[1].path);
        REQUIRE(mesh.tangents.size() == mesh.positions.size());
        for (std::size_t i = 0; i < mesh.tangents.size(); i++) {
            checkUnitTangent(mesh.tangents[i], mesh.normals[i]);
        }
        checkBitangentsFollowUv(mesh);
    }
}

TEST_CASE("an OBJ's tangents are made in its own UV space, with no glTF negation", "[editor][assets][import][tangents]") {
    ModelImportFixture fixture{"importObjTangents"};
    fixture.scratch.writeFile("assets/client/Quad.obj", TechEngineTests::OBJ_QUAD_WITH_UVS);

    ModelImportResult result;
    REQUIRE(fixture.importFile("Quad.obj", ImportSettings{}, result) == std::error_code{});
    REQUIRE(result.records.size() == 3);
    const Mesh mesh = fixture.readMesh(result.records[1].path);
    REQUIRE(mesh.tangents.size() == mesh.positions.size());

    for (std::size_t i = 0; i < mesh.tangents.size(); i++) {
        checkVec3(mesh.normals[i], Vec3(0.0f, 0.0f, 1.0f));
        checkVec3(Vec3(mesh.tangents[i]), Vec3(1.0f, 0.0f, 0.0f));
        CHECK(mesh.tangents[i].w == 1.0f);
    }
    checkBitangentsFollowUv(mesh);
}

TEST_CASE("a quantized or meshopt-compressed glTF is refused before assimp reads it", "[editor][assets][import][extensions]") {
    ModelImportFixture fixture{"importRefusesCompressed"};
    std::string fileName = "Pair.gltf";

    SECTION("KHR_mesh_quantization") {
        fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(withExtensionsUsed(R"(["KHR_mesh_quantization"])", TechEngineTests::GLTF_PAIR_BODY)));
    }

    SECTION("EXT_meshopt_compression, listed after an extension that imports") {
        fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(withExtensionsUsed(R"([ "KHR_materials_sheen" , "EXT_meshopt_compression" ])", TechEngineTests::GLTF_PAIR_BODY)));
    }

    SECTION("KHR_meshopt_compression") {
        fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(withExtensionsUsed(R"(["KHR_meshopt_compression"])", TechEngineTests::GLTF_PAIR_BODY)));
    }

    SECTION("a file that assimp could not parse at all, so the refusal came first") {
        fixture.scratch.writeFile("assets/client/Pair.gltf", R"({"asset":{"version":"2.0"},"extensionsUsed":["KHR_mesh_quantization"],)");
    }

    SECTION("an extension in upper case") {
        fileName = "Pair.GLTF";
        fixture.scratch.writeFile("assets/client/Pair.GLTF", TechEngineTests::gltfFile(withExtensionsUsed(R"(["KHR_mesh_quantization"])", TechEngineTests::GLTF_PAIR_BODY)));
    }

    SECTION("a binary glTF") {
        fileName = "Pair.glb";
        fixture.scratch.writeFile("assets/client/Pair.glb", TechEngineTests::glbFile(TechEngineTests::gltfFile(withExtensionsUsed(R"(["KHR_mesh_quantization"])", TechEngineTests::GLTF_PAIR_BODY))));
    }

    ModelImportResult result;
    CHECK(fixture.importFile(fileName, ImportSettings{}, result) == ProjectError::SourceCompressed);
    CHECK(result.records.empty());
    CHECK(result.folder.empty());
    CHECK_FALSE(fixture.exists("assets/client/Pair"));
    CHECK(fixture.nextId == FIRST_TEST_ID);
}

TEST_CASE("a refused extension's name outside extensionsUsed does not refuse the file", "[editor][assets][import][extensions]") {
    ModelImportFixture fixture{"importNamesNotExtensions"};

    SECTION("a glTF whose node and mesh carry the names") {
        const std::string body = withExtensionsUsed(
            R"(["KHR_materials_sheen"])",
            R"("scenes":[{"nodes":[0]}],)"
            R"("nodes":[{"name":"KHR_mesh_quantization","mesh":0}],)"
            R"("meshes":[{"name":"EXT_meshopt_compression","primitives":[{"attributes":{"POSITION":1},"indices":0}]}])");
        fixture.scratch.writeFile("assets/client/Named.gltf", TechEngineTests::gltfFile(body));

        ModelImportResult result;
        CHECK(fixture.importFile("Named.gltf", ImportSettings{}, result) == std::error_code{});
        CHECK(result.records.size() == 3);
    }

    SECTION("a binary glTF that lists only an extension that imports") {
        fixture.scratch.writeFile("assets/client/Named.glb", TechEngineTests::glbFile(TechEngineTests::gltfFile(withExtensionsUsed(R"(["KHR_materials_sheen"])", TechEngineTests::GLTF_PAIR_BODY))));

        ModelImportResult result;
        CHECK(fixture.importFile("Named.glb", ImportSettings{}, result) != ProjectError::SourceCompressed);
    }
}

TEST_CASE("an import scale that is not a finite number above zero is refused before anything is read", "[editor][assets][import]") {
    ModelImportFixture fixture{"importRefusesScale"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    ImportSettings settings;

    SECTION("zero") {
        settings.scale = 0.0f;
    }

    SECTION("negative zero") {
        settings.scale = -0.0f;
    }

    SECTION("a negative scale, which would mirror the model") {
        settings.scale = -1.0f;
    }

    SECTION("not a number") {
        settings.scale = std::numeric_limits<float>::quiet_NaN();
    }

    SECTION("infinity") {
        settings.scale = std::numeric_limits<float>::infinity();
    }

    ModelImportResult result;
    CHECK(fixture.importFile("Pair.gltf", settings, result) == ProjectError::InvalidImportScale);
    CHECK(result.records.empty());
    CHECK_FALSE(fixture.exists("assets/client/Pair"));
    CHECK(fixture.nextId == FIRST_TEST_ID);
}

TEST_CASE("the smallest positive scale is accepted", "[editor][assets][import]") {
    ModelImportFixture fixture{"importTinyScale"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    ModelImportResult result;
    CHECK(fixture.importFile("Pair.gltf", ImportSettings{std::numeric_limits<float>::min(), UpAxis::Y}, result) == std::error_code{});
}
