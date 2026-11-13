#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ModelTestFiles.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::ImportMode;
using TechEngine::ImportSettings;
using TechEngine::Mesh;
using TechEngine::MeshSlot;
using TechEngine::ModelImporter;
using TechEngine::ModelImportRequest;
using TechEngine::ModelImportResult;
using TechEngine::ProjectError;
using TechEngine::ResourceId;
using TechEngine::UpAxis;
using TechEngine::Vec2;
using TechEngine::Vec3;
using TechEngine::Vec4;
using TechEngineTests::ScratchDirectory;

static constexpr std::uint64_t FIRST_TEST_ID = 0x2000;
static constexpr float TOLERANCE = 1.0e-5f;

class MergedImportFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    std::uint64_t nextId = FIRST_TEST_ID;

    explicit MergedImportFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    std::error_code importFile(const std::string_view fileName, const ImportSettings& settings, ModelImportResult& out) {
        ModelImportRequest request;
        request.sourcePath = "project://assets/client/" + std::string(fileName);
        request.side = TechEngine::Side::Client;
        request.settings = settings;
        request.mode = ImportMode::MergedMesh;
        const ModelImporter importer(files, [this] {
            return ResourceId{nextId++};
        });
        return importer.importFile(request, out);
    }

    Mesh importMesh(const std::string_view fileName, const std::string& contents, const ImportSettings& settings = ImportSettings{}) {
        scratch.writeFile("assets/client/" + std::string(fileName), contents);
        ModelImportResult result;
        REQUIRE(importFile(fileName, settings, result) == std::error_code{});
        REQUIRE(result.records.size() == 1);
        return readMesh(result.records[0].path);
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

static void checkVec3(const Vec3 actual, const Vec3 expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(TOLERANCE));
    CHECK(actual.y == Catch::Approx(expected.y).margin(TOLERANCE));
    CHECK(actual.z == Catch::Approx(expected.z).margin(TOLERANCE));
}

static void checkVec2(const Vec2 actual, const Vec2 expected) {
    CHECK(actual.x == Catch::Approx(expected.x).margin(TOLERANCE));
    CHECK(actual.y == Catch::Approx(expected.y).margin(TOLERANCE));
}

static Vec3 faceNormal(const Mesh& mesh, const std::size_t firstIndex) {
    const Vec3 a = mesh.positions[mesh.indices[firstIndex]];
    const Vec3 b = mesh.positions[mesh.indices[firstIndex + 1]];
    const Vec3 c = mesh.positions[mesh.indices[firstIndex + 2]];
    return glm::normalize(glm::cross(b - a, c - a));
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

// Every corner's frame is unit and orthogonal, its normal is on the front of its triangle, and cross(n, t.xyz) * w
// points the way V grows in the stored UV0.
static void checkFrames(const Mesh& mesh) {
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    REQUIRE(mesh.tangents.size() == mesh.positions.size());
    REQUIRE(mesh.uv0.size() == mesh.positions.size());
    for (std::size_t first = 0; first < mesh.indices.size(); first += 3) {
        const Vec3 front = faceNormal(mesh, first);
        const Vec3 bitangent = uvBitangent(mesh, first);
        for (std::size_t corner = 0; corner < 3; corner++) {
            const std::uint32_t vertex = mesh.indices[first + corner];
            const Vec3 normal = mesh.normals[vertex];
            const Vec4 tangent = mesh.tangents[vertex];
            CHECK(glm::length(normal) == Catch::Approx(1.0f).margin(TOLERANCE));
            CHECK(glm::length(Vec3(tangent)) == Catch::Approx(1.0f).margin(TOLERANCE));
            CHECK(glm::dot(normal, Vec3(tangent)) == Catch::Approx(0.0f).margin(TOLERANCE));
            CHECK(glm::dot(normal, front) > 0.0f);
            CHECK(glm::dot(glm::cross(normal, Vec3(tangent)) * tangent.w, bitangent) > 0.0f);
        }
    }
}

static std::ptrdiff_t entryCount(const std::filesystem::path& directory) {
    return std::distance(std::filesystem::directory_iterator{directory}, std::filesystem::directory_iterator{});
}

TEST_CASE("the root asset is named after the folder, a model or a merged mesh by the mode", "[editor][assets][import][merged]") {
    CHECK(ModelImporter::rootAssetPath("project://assets/client/Sponza", ImportMode::Model) == "project://assets/client/Sponza/Sponza.temodel");
    CHECK(ModelImporter::rootAssetPath("project://assets/client/Sponza", ImportMode::MergedMesh) == "project://assets/client/Sponza/Sponza.temesh");
}

TEST_CASE("a merged import writes one root mesh with an empty key and the real settings, and no model", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedRootAsset"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    const ImportSettings settings{0.01f, UpAxis::Z};

    ModelImportResult result;
    REQUIRE(fixture.importFile("Pair.gltf", settings, result) == std::error_code{});

    CHECK(result.folder == "project://assets/client/Pair");
    CHECK(result.nodeCount == 0);
    REQUIRE(result.records.size() == 1);
    CHECK(result.records[0].typeTag == TechEngine::MESH_TYPE_TAG);
    CHECK(result.records[0].side == TechEngine::Side::Client);
    CHECK(result.records[0].path == "project://assets/client/Pair/Pair.temesh");
    CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair") == 1);
    CHECK_FALSE(fixture.exists("assets/client/Pair/Pair.temodel"));
    CHECK_FALSE(fixture.exists("assets/client/Pair/Meshes"));
    CHECK(fixture.nextId == FIRST_TEST_ID + 1);

    const Mesh mesh = fixture.readMesh(result.records[0].path);
    CHECK(mesh.id == result.records[0].id);
    CHECK(mesh.importBlock.sourcePath == "assets/client/Pair.gltf");
    CHECK(mesh.importBlock.sourceIndex == 0);
    CHECK(mesh.importBlock.sourceName.empty());
    CHECK(mesh.importBlock.settings == settings);
}

TEST_CASE("a merged import bakes each node's world transform, the scale and the up axis into the vertices", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedWorldBake"};
    const Mesh mesh = fixture.importMesh("Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY), ImportSettings{0.01f, UpAxis::Z});

    REQUIRE(mesh.positions.size() == 6);
    REQUIRE(mesh.indices.size() == 6);
    REQUIRE(mesh.slots.size() == 1);
    CHECK(mesh.slots[0].firstIndex == 0);
    CHECK(mesh.slots[0].indexCount == 6);

    SECTION("the body's triangle comes first, moved 5 along Z, then turned and scaled") {
        checkVec3(mesh.positions[0], Vec3(0.01f, 0.08f, -0.02f));
        checkVec3(mesh.positions[1], Vec3(0.04f, 0.08f, -0.02f));
        checkVec3(mesh.positions[2], Vec3(0.01f, 0.08f, -0.05f));
    }

    SECTION("the wheel's vertex lands where the model's placed tree puts it") {
        // In the source: scale (4, 2, 3) by (1, 2, 3), turn a quarter about Z, then move 5 along Z.
        const Vec3 source = Vec3(-4.0f, 4.0f, 14.0f);
        checkVec3(mesh.positions[4], Vec3(source.x, source.z, -source.y) * 0.01f);
    }

    SECTION("nothing is mirrored, so no triangle is reversed") {
        CHECK(mesh.indices == std::vector<std::uint32_t>{0, 1, 2, 3, 4, 5});
    }

    SECTION("each normal is unit and faces the front of its triangle, and no UV0 means no UV0 or tangents") {
        REQUIRE(mesh.normals.size() == mesh.positions.size());
        for (std::size_t first = 0; first < mesh.indices.size(); first += 3) {
            const Vec3 front = faceNormal(mesh, first);
            for (std::size_t corner = 0; corner < 3; corner++) {
                checkVec3(mesh.normals[mesh.indices[first + corner]], front);
            }
        }
        CHECK(mesh.uv0.empty());
        CHECK(mesh.tangents.empty());
    }
}

TEST_CASE("a node scaled -1 on X gives the mirrored half's triangles in reversed order", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedMirror"};
    const Mesh mesh = fixture.importMesh("Mirror.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_MIRRORED_QUAD_BODY));

    REQUIRE(mesh.positions.size() == 8);
    REQUIRE(mesh.indices.size() == 12);
    REQUIRE(mesh.slots.size() == 1);
    CHECK(mesh.slots[0].indexCount == 12);
    constexpr std::uint32_t half = 4;

    SECTION("the second copy is the first one mirrored") {
        for (std::uint32_t i = 0; i < half; i++) {
            CHECK(mesh.positions[i].x > 0.0f);
            const Vec3 original = mesh.positions[i];
            checkVec3(mesh.positions[half + i], Vec3(-original.x, original.y, original.z));
        }
    }

    SECTION("each mirrored triangle names the same corners with its last two swapped") {
        for (std::size_t first = 0; first < 6; first += 3) {
            CHECK(mesh.indices[6 + first] == mesh.indices[first] + half);
            CHECK(mesh.indices[6 + first + 1] == mesh.indices[first + 2] + half);
            CHECK(mesh.indices[6 + first + 2] == mesh.indices[first + 1] + half);
        }
    }

    SECTION("the mirrored tangent points along -X and its w is flipped") {
        for (std::uint32_t i = 0; i < half; i++) {
            checkVec3(Vec3(mesh.tangents[i]), Vec3(1.0f, 0.0f, 0.0f));
            CHECK(mesh.tangents[i].w == -1.0f);
            checkVec3(Vec3(mesh.tangents[half + i]), Vec3(-1.0f, 0.0f, 0.0f));
            CHECK(mesh.tangents[half + i].w == 1.0f);
        }
    }

    SECTION("both halves face front, and their tangents build the bitangent V follows") {
        checkFrames(mesh);
    }
}

TEST_CASE("under a scale that is not along the mesh's axes, normals and tangents stay a frame", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedStretch"};
    const Mesh mesh = fixture.importMesh("Stretch.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_STRETCHED_QUAD_BODY));

    REQUIRE(mesh.positions.size() == 4);
    checkFrames(mesh);
    const float halfRoot = std::sqrt(0.5f);
    checkVec3(Vec3(mesh.tangents[0]), glm::normalize(Vec3(halfRoot, 2.0f * halfRoot, 0.0f)));
}

TEST_CASE("two materials give two slots in material-index order", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedMaterials"};
    const Mesh mesh = fixture.importMesh("Front.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_MATERIALS_BODY));

    REQUIRE(mesh.slots.size() == 2);
    REQUIRE(mesh.indices.size() == 6);

    SECTION("each slot is named after its material, and a material no mesh uses gets none") {
        CHECK(mesh.slots[0].name == "Paint");
        CHECK(mesh.slots[1].name == "Glass");
        CHECK(mesh.slots[0] == MeshSlot{"Paint", 0, 3, ResourceId{}});
        CHECK(mesh.slots[1] == MeshSlot{"Glass", 3, 3, ResourceId{}});
    }

    SECTION("the first slot holds the door's triangle, though the walk met the window first") {
        for (std::size_t i = 0; i < 3; i++) {
            CHECK(mesh.positions[mesh.indices[i]].z == Catch::Approx(13.0f));
            CHECK(mesh.positions[mesh.indices[3 + i]].z == Catch::Approx(3.0f));
        }
    }
}

TEST_CASE("two materials with one name give two slots with different names", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedSameNames"};
    const Mesh mesh = fixture.importMesh("Front.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_SAME_NAMED_MATERIALS_BODY));

    REQUIRE(mesh.slots.size() == 2);
    CHECK(mesh.slots[0].name == "Paint");
    CHECK(mesh.slots[1].name == "Paint_1");
}

TEST_CASE("a part without UV0 joins with zero UVs and the tangent (1, 0, 0, 1)", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedMissingUvs"};
    const Mesh mesh = fixture.importMesh("Parts.gltf", TechEngineTests::gltfQuadFile(TechEngineTests::GLTF_QUAD_PART_WITHOUT_UVS_BODY));

    REQUIRE(mesh.slots.size() == 1);
    REQUIRE(mesh.uv0.size() == mesh.positions.size());
    REQUIRE(mesh.tangents.size() == mesh.positions.size());
    REQUIRE(mesh.normals.size() == mesh.positions.size());
    std::size_t bareVertices = 0;
    for (std::size_t i = 0; i < mesh.positions.size(); i++) {
        const Vec3 position = mesh.positions[i];
        if (position.z > 10.0f) {
            bareVertices++;
            checkVec2(mesh.uv0[i], Vec2(0.0f, 0.0f));
            CHECK(mesh.tangents[i] == Vec4(1.0f, 0.0f, 0.0f, 1.0f));
        } else {
            checkVec2(mesh.uv0[i], Vec2((position.x - 1.0f) / 3.0f, 1.0f - (position.y - 2.0f) / 3.0f));
            CHECK(glm::dot(mesh.normals[i], Vec3(mesh.tangents[i])) == Catch::Approx(0.0f).margin(TOLERANCE));
        }
        checkVec3(mesh.normals[i], TechEngineTests::QUAD_NORMAL);
    }
    CHECK(bareVertices == 4);
}

TEST_CASE("a merged import makes missing normals smooth across a shared edge", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedSmoothNormals"};
    const Mesh mesh = fixture.importMesh("Bent.obj", std::string(TechEngineTests::OBJ_BENT));

    REQUIRE(mesh.positions.size() == 4);
    REQUIRE(mesh.normals.size() == 4);
    const float halfRoot = std::sqrt(0.5f);
    for (std::size_t i = 0; i < mesh.positions.size(); i++) {
        const Vec3 position = mesh.positions[i];
        if (position.y == 0.0f && position.z == 0.0f) {
            checkVec3(mesh.normals[i], Vec3(0.0f, halfRoot, halfRoot));
        }
    }
}

TEST_CASE("a merged import that cannot be written leaves no folder behind", "[editor][assets][import][merged]") {
    MergedImportFixture fixture{"mergedRefusals"};
    ModelImportResult result;

    SECTION("a file with points and no triangles") {
        fixture.scratch.writeFile("assets/client/Dots.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_POINTS_ONLY_BODY));
        CHECK(fixture.importFile("Dots.gltf", ImportSettings{}, result) == ProjectError::SourceHasNoMeshes);
        CHECK_FALSE(fixture.exists("assets/client/Dots"));
    }

    SECTION("a folder that exists") {
        fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
        fixture.scratch.makeDirectory("assets/client/Pair");
        CHECK(fixture.importFile("Pair.gltf", ImportSettings{}, result) == ProjectError::ImportFolderExists);
        CHECK(entryCount(fixture.scratch.root() / "assets" / "client" / "Pair") == 0);
    }

    CHECK(result.records.empty());
    CHECK(fixture.nextId == FIRST_TEST_ID);
}
