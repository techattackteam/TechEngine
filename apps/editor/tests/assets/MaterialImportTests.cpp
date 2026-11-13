#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
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

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::ImportMode;
using TechEngine::ImportSettings;
using TechEngine::Material;
using TechEngine::MaterialAlphaMode;
using TechEngine::MaterialShadingModel;
using TechEngine::Mesh;
using TechEngine::ModelImportRequest;
using TechEngine::ModelImportResult;
using TechEngine::ResourceId;
using TechEngine::StringId;
using TechEngine::Texture;
using TechEngine::TextureFilter;
using TechEngine::TextureFormat;
using TechEngine::TextureWrap;
using TechEngine::Vec3;
using TechEngine::Vec4;
using TechEngineTests::ScratchDirectory;

static constexpr std::uint64_t FIRST_TEST_ID = 0x3000;
static constexpr float TOLERANCE = 1.0e-5f;
static constexpr int BC_TOLERANCE = 4;

class MaterialImportFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    std::uint64_t nextId = FIRST_TEST_ID;
    ModelImportResult result;

    explicit MaterialImportFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    void importFile(const std::string_view fileName, const ImportMode mode = ImportMode::Model) {
        ModelImportRequest request;
        request.sourcePath = "project://assets/client/" + std::string(fileName);
        request.side = TechEngine::Side::Client;
        request.settings = ImportSettings{};
        request.mode = mode;
        const TechEngine::ModelImporter importer(files, [this] {
            return ResourceId{nextId++};
        });
        REQUIRE(importer.importFile(request, result) == std::error_code{});
    }

    std::vector<AssetRecord> recordsOf(const StringId typeTag) const {
        std::vector<AssetRecord> found;
        std::ranges::copy_if(result.records, std::back_inserter(found), [typeTag](const AssetRecord& record) {
            return record.typeTag == typeTag;
        });
        return found;
    }

    Material readMaterial(const std::string& path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Material material;
        REQUIRE(material.read(bytes) == std::error_code{});
        return material;
    }

    Texture readTexture(const std::string& path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Texture texture;
        REQUIRE(texture.read(bytes) == std::error_code{});
        return texture;
    }

    Mesh readMesh(const std::string& path) const {
        std::vector<std::byte> bytes;
        REQUIRE(files.read(path, bytes) == std::error_code{});
        Mesh mesh;
        REQUIRE(mesh.read(bytes) == std::error_code{});
        return mesh;
    }

    Material materialNamed(const std::string_view sourceName) const {
        for (const AssetRecord& record: recordsOf(TechEngine::MATERIAL_TYPE_TAG)) {
            const Material material = readMaterial(record.path);
            if (material.importBlock.sourceName == sourceName) {
                return material;
            }
        }
        FAIL("no material named " << sourceName);
        return Material{};
    }

    Texture textureKeyed(const std::string_view sourceName) const {
        for (const AssetRecord& record: recordsOf(TechEngine::TEXTURE_TYPE_TAG)) {
            const Texture texture = readTexture(record.path);
            if (texture.importBlock.sourceName == sourceName) {
                return texture;
            }
        }
        FAIL("no texture keyed " << sourceName);
        return Texture{};
    }

    bool exists(const std::string_view relative) const {
        return std::filesystem::exists(scratch.root() / relative);
    }
};

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

static bool near(const int actual, const int expected) {
    return std::abs(actual - expected) <= BC_TOLERANCE;
}

static void checkTopTexel(const Texture& texture, const std::array<int, 3> expected) {
    const std::vector<std::uint8_t> pixels = TechEngineTests::decodeLevel(texture, 0);
    for (std::size_t pixel = 0; pixel < pixels.size(); pixel += 4) {
        CHECK(near(pixels[pixel], expected[0]));
        CHECK(near(pixels[pixel + 1], expected[1]));
        CHECK(near(pixels[pixel + 2], expected[2]));
    }
}

static void checkFullChain(const Texture& texture) {
    REQUIRE(texture.mips.size() == Texture::mipCount(texture.width, texture.height));
    for (std::uint32_t level = 0; level < texture.mips.size(); level++) {
        CHECK(texture.mips[level].size() == Texture::mipByteSize(texture.format, texture.width, texture.height, level));
    }
}

// A binary glTF whose one image is a PNG in the BIN chunk, and whose geometry is gltfFile's triangle in a data URI.
static std::string glbWithEmbeddedPng(const std::string& png) {
    const std::string length = std::to_string(png.size());
    std::string json = R"({"asset":{"version":"2.0"},"scene":0,"scenes":[{"nodes":[0]}],"nodes":[{"name":"Part","mesh":0}],)";
    json += R"("meshes":[{"name":"Part","primitives":[{"attributes":{"POSITION":1},"indices":0,"material":0}]}],)";
    json += R"("materials":[{"name":"Painted","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],)";
    json += R"("textures":[{"source":0}],"images":[{"bufferView":2,"mimeType":"image/png"}],)";
    json += R"("accessors":[{"bufferView":0,"componentType":5123,"count":3,"type":"SCALAR"},)";
    json += R"({"bufferView":1,"componentType":5126,"count":3,"type":"VEC3","min":[1,2,3],"max":[4,5,3]}],)";
    json += R"("bufferViews":[{"buffer":1,"byteOffset":0,"byteLength":6,"target":34963},{"buffer":1,"byteOffset":8,"byteLength":36,"target":34962},)";
    json += R"({"buffer":0,"byteOffset":0,"byteLength":)" + length + "}],";
    json += R"("buffers":[{"byteLength":)" + length + R"(},{"byteLength":44,"uri":"data:application/octet-stream;base64,AAABAAIAAAAAAIA/AAAAQAAAQEAAAIBAAAAAQAAAQEAAAIA/AACgQAAAQEA="}]})";
    while (json.size() % 4 != 0) {
        json.push_back(' ');
    }
    std::string binary = png;
    while (binary.size() % 4 != 0) {
        binary.push_back('\0');
    }

    std::string file = "glTF";
    TechEngineTests::appendU32(file, 2);
    TechEngineTests::appendU32(file, static_cast<std::uint32_t>(12 + 8 + json.size() + 8 + binary.size()));
    TechEngineTests::appendU32(file, static_cast<std::uint32_t>(json.size()));
    file += "JSON";
    file += json;
    TechEngineTests::appendU32(file, static_cast<std::uint32_t>(binary.size()));
    file += std::string("BIN\0", 4);
    file += binary;
    return file;
}

TEST_CASE("two materials that share a base color image write that texture once", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsShareTexture"};
    fixture.scratch.writeFile("assets/client/Textures/wall.png", TechEngineTests::solidPng(4, 4, {200, 100, 50, 255}));
    fixture.scratch.writeFile(
        "assets/client/Wall.gltf",
        materialsGltf(
            4,
            R"("materials":[{"name":"Brick","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}},)"
            R"({"name":"Stone","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}},)"
            R"({"name":"Bump","normalTexture":{"index":0}},)"
            R"({"name":"Clamped","pbrMetallicRoughness":{"baseColorTexture":{"index":1}}}],)"
            R"("textures":[{"source":0},{"source":0,"sampler":0}],)"
            R"("samplers":[{"wrapS":33071,"wrapT":33071,"magFilter":9728}],)"
            R"("images":[{"uri":"Textures/wall.png"}])"));
    fixture.importFile("Wall.gltf");

    const std::vector<AssetRecord> textures = fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG);
    REQUIRE(textures.size() == 3);
    const Material brick = fixture.materialNamed("Brick");
    const Material stone = fixture.materialNamed("Stone");

    SECTION("both materials name the one texture") {
        const Texture shared = fixture.textureKeyed("Textures/wall.png#basecolor#repeat-linear");
        CHECK(brick.baseColorTexture.id == shared.id);
        CHECK(stone.baseColorTexture.id == shared.id);
        CHECK(shared.format == TextureFormat::Bc7Srgb);
        CHECK(shared.importBlock.sourcePath == "assets/client/Wall.gltf");
        CHECK(shared.importBlock.sourceIndex == 0);
        CHECK(shared.importBlock.settings == ImportSettings{});
        checkTopTexel(shared, {200, 100, 50});
    }

    SECTION("the same image for another purpose, or with another sampler, is a second file") {
        const Texture normal = fixture.textureKeyed("Textures/wall.png#normal#repeat-linear");
        const Texture clamped = fixture.textureKeyed("Textures/wall.png#basecolor#clamp-nearest");
        CHECK(fixture.materialNamed("Bump").normalTexture.id == normal.id);
        CHECK(fixture.materialNamed("Clamped").baseColorTexture.id == clamped.id);
        CHECK(normal.format == TextureFormat::Bc5Unorm);
        CHECK(clamped.sampler == TechEngine::TextureSampler{TextureWrap::Clamp, TextureWrap::Clamp, TextureFilter::Nearest});
        CHECK(normal.id != clamped.id);
        CHECK(normal.id != brick.baseColorTexture.id);
    }

    SECTION("the files sit in Textures, named after the image and the purpose") {
        CHECK(fixture.exists("assets/client/Wall/Textures/wall_basecolor.tetexture"));
        CHECK(fixture.exists("assets/client/Wall/Textures/wall_normal.tetexture"));
        CHECK(fixture.exists("assets/client/Wall/Textures/wall_basecolor_1.tetexture"));
        for (const AssetRecord& record: textures) {
            CHECK(record.side == TechEngine::Side::Client);
            CHECK(fixture.readTexture(record.path).id == record.id);
        }
    }
}

TEST_CASE("separate occlusion and metallic-roughness images give one packed ORM texture", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsPackOrm"};
    fixture.scratch.writeFile("assets/client/Textures/ao.png", TechEngineTests::solidPng(2, 2, {200, 0, 0, 255}));
    fixture.scratch.writeFile("assets/client/Textures/mr.png", TechEngineTests::solidPng(4, 4, {0, 100, 50, 255}));
    fixture.scratch.writeFile("assets/client/Textures/orm.png", TechEngineTests::solidPng(4, 4, {30, 60, 90, 255}));

    SECTION("red from occlusion, green and blue from metallic-roughness, at the larger size") {
        fixture.scratch.writeFile(
            "assets/client/Rock.gltf",
            materialsGltf(
                1,
                R"("materials":[{"name":"Rock","occlusionTexture":{"index":0},"pbrMetallicRoughness":{"metallicRoughnessTexture":{"index":1}}}],)"
                R"("textures":[{"source":0},{"source":1}],"images":[{"uri":"Textures/ao.png"},{"uri":"Textures/mr.png"}])"));
        fixture.importFile("Rock.gltf");

        REQUIRE(fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG).size() == 1);
        const Texture packed = fixture.textureKeyed("Textures/ao.png+Textures/mr.png#orm#repeat-linear");
        CHECK(fixture.materialNamed("Rock").occlusionRoughnessMetallicTexture.id == packed.id);
        CHECK(packed.format == TextureFormat::Bc7Unorm);
        CHECK(packed.width == 4);
        CHECK(packed.height == 4);
        checkTopTexel(packed, {200, 100, 50});
        CHECK(fixture.exists("assets/client/Rock/Textures/ao_orm.tetexture"));
    }

    SECTION("a material with no occlusion gives a red of 1") {
        fixture.scratch.writeFile(
            "assets/client/Rock.gltf",
            materialsGltf(
                1,
                R"("materials":[{"name":"Rock","pbrMetallicRoughness":{"metallicRoughnessTexture":{"index":0}}}],)"
                R"("textures":[{"source":0}],"images":[{"uri":"Textures/mr.png"}])"));
        fixture.importFile("Rock.gltf");

        const Texture packed = fixture.textureKeyed("+Textures/mr.png#orm#repeat-linear");
        CHECK(fixture.materialNamed("Rock").occlusionRoughnessMetallicTexture.id == packed.id);
        checkTopTexel(packed, {255, 100, 50});
    }

    SECTION("one image for both is used as it is") {
        fixture.scratch.writeFile(
            "assets/client/Rock.gltf",
            materialsGltf(
                1,
                R"("materials":[{"name":"Rock","occlusionTexture":{"index":0},"pbrMetallicRoughness":{"metallicRoughnessTexture":{"index":0}}}],)"
                R"("textures":[{"source":0}],"images":[{"uri":"Textures/orm.png"}])"));
        fixture.importFile("Rock.gltf");

        const Texture packed = fixture.textureKeyed("Textures/orm.png#orm#repeat-linear");
        checkTopTexel(packed, {30, 60, 90});
    }
}

TEST_CASE("a .glb fixture's embedded PNG is imported", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsEmbeddedPng"};
    fixture.scratch.writeFile("assets/client/Crate.glb", glbWithEmbeddedPng(TechEngineTests::solidPng(8, 4, {10, 220, 90, 255})));
    fixture.importFile("Crate.glb");

    const Texture texture = fixture.textureKeyed("*0#basecolor#repeat-linear");
    CHECK(fixture.materialNamed("Painted").baseColorTexture.id == texture.id);
    CHECK(texture.width == 8);
    CHECK(texture.height == 4);
    CHECK(texture.importBlock.sourcePath == "assets/client/Crate.glb");
    checkTopTexel(texture, {10, 220, 90});
    CHECK(fixture.exists("assets/client/Crate/Textures/Texture 0_basecolor.tetexture"));
}

TEST_CASE("a BC1 DDS fixture is decoded and written as BC7", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsDds"};
    std::string blocks;
    for (int i = 0; i < 4; i++) {
        blocks += TechEngineTests::solidBc1Block(0xF800);
    }
    fixture.scratch.writeFile("assets/client/Textures/rock.dds", TechEngineTests::ddsFile(8, 8, "DXT1", blocks));
    fixture.scratch.writeFile(
        "assets/client/Rock.gltf",
        materialsGltf(
            1,
            R"("materials":[{"name":"Rock","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}}],)"
            R"("textures":[{"source":0}],"images":[{"uri":"Textures/rock.dds"}])"));
    fixture.importFile("Rock.gltf");

    const Texture texture = fixture.textureKeyed("Textures/rock.dds#basecolor#repeat-linear");
    CHECK(texture.format == TextureFormat::Bc7Srgb);
    CHECK(texture.width == 8);
    CHECK(texture.height == 8);
    checkFullChain(texture);
    checkTopTexel(texture, {255, 0, 0});
}

TEST_CASE("each texture has its input's format and a full mip chain", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsFormats"};
    fixture.scratch.writeFile("assets/client/Textures/base.png", TechEngineTests::solidPng(16, 8, {255, 255, 255, 255}));
    fixture.scratch.writeFile("assets/client/Textures/mr.png", TechEngineTests::solidPng(5, 3, {0, 128, 0, 255}));
    fixture.scratch.writeFile("assets/client/Textures/normal.png", TechEngineTests::solidPng(8, 8, {128, 128, 255, 255}));
    fixture.scratch.writeFile("assets/client/Textures/glow.png", TechEngineTests::solidPng(1, 1, {255, 0, 0, 255}));
    fixture.scratch.writeFile(
        "assets/client/Suit.gltf",
        materialsGltf(
            1,
            R"("materials":[{"name":"Suit","pbrMetallicRoughness":{"baseColorTexture":{"index":0},"metallicRoughnessTexture":{"index":1}},)"
            R"("normalTexture":{"index":2},"emissiveTexture":{"index":3}}],)"
            R"("textures":[{"source":0},{"source":1},{"source":2},{"source":3}],)"
            R"("images":[{"uri":"Textures/base.png"},{"uri":"Textures/mr.png"},{"uri":"Textures/normal.png"},{"uri":"Textures/glow.png"}])"));
    fixture.importFile("Suit.gltf");

    REQUIRE(fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG).size() == 4);
    const Material suit = fixture.materialNamed("Suit");
    const Texture baseColor = fixture.textureKeyed("Textures/base.png#basecolor#repeat-linear");
    const Texture packed = fixture.textureKeyed("+Textures/mr.png#orm#repeat-linear");
    const Texture normal = fixture.textureKeyed("Textures/normal.png#normal#repeat-linear");
    const Texture emissive = fixture.textureKeyed("Textures/glow.png#emissive#repeat-linear");

    CHECK(baseColor.format == TextureFormat::Bc7Srgb);
    CHECK(packed.format == TextureFormat::Bc7Unorm);
    CHECK(normal.format == TextureFormat::Bc5Unorm);
    CHECK(emissive.format == TextureFormat::Bc7Srgb);
    CHECK(suit.baseColorTexture.id == baseColor.id);
    CHECK(suit.occlusionRoughnessMetallicTexture.id == packed.id);
    CHECK(suit.normalTexture.id == normal.id);
    CHECK(suit.emissiveTexture.id == emissive.id);
    for (const Texture* texture: {&baseColor, &packed, &normal, &emissive}) {
        CHECK(texture->shape == TechEngine::TextureShape::Texture2D);
        checkFullChain(*texture);
    }
    CHECK(packed.width == 5);
    CHECK(packed.height == 3);
    CHECK(emissive.mips.size() == 1);
}

TEST_CASE("each texture's source key names its path, purpose and sampler", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsKeys"};
    fixture.scratch.writeFile("assets/client/Maps/Wall Tile.png", TechEngineTests::solidPng(2, 2, {1, 2, 3, 255}));
    fixture.scratch.writeFile(
        "assets/client/Wall.gltf",
        materialsGltf(
            2,
            R"("materials":[{"name":"Later","normalTexture":{"index":0}},)"
            R"({"name":"Earlier","emissiveTexture":{"index":1}}],)"
            R"("textures":[{"source":0,"sampler":0},{"source":0}],)"
            R"("samplers":[{"wrapS":33648,"wrapT":10497}],)"
            R"("images":[{"uri":"Maps/Wall%20Tile.png"}])"));
    fixture.importFile("Wall.gltf");

    std::vector<std::string> keys;
    for (const AssetRecord& record: fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG)) {
        keys.push_back(fixture.readTexture(record.path).importBlock.sourceName);
    }
    std::ranges::sort(keys);
    CHECK(keys == std::vector<std::string>{"Maps/Wall Tile.png#emissive#repeat-linear", "Maps/Wall Tile.png#normal#mirror-repeat-linear"});
    CHECK(fixture.textureKeyed("Maps/Wall Tile.png#normal#mirror-repeat-linear").sampler == TechEngine::TextureSampler{TextureWrap::Mirror, TextureWrap::Repeat, TextureFilter::Linear});
}

TEST_CASE("each mesh slot's default is its material's id", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsSlotDefaults"};
    fixture.scratch.writeFile("assets/client/Front.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_TWO_MATERIALS_BODY));

    SECTION("a model's meshes") {
        fixture.importFile("Front.gltf");
        const std::vector<AssetRecord> meshes = fixture.recordsOf(TechEngine::MESH_TYPE_TAG);
        REQUIRE(meshes.size() == 2);
        for (const AssetRecord& record: meshes) {
            const Mesh mesh = fixture.readMesh(record.path);
            REQUIRE(mesh.slots.size() == 1);
            CHECK(mesh.slots[0].defaultMaterial == fixture.materialNamed(mesh.slots[0].name).id);
        }
    }

    SECTION("a merged mesh's slots") {
        fixture.importFile("Front.gltf", ImportMode::MergedMesh);
        REQUIRE(fixture.result.records[0].typeTag == TechEngine::MESH_TYPE_TAG);
        const Mesh mesh = fixture.readMesh(fixture.result.records[0].path);
        REQUIRE(mesh.slots.size() == 2);
        CHECK(mesh.slots[0].defaultMaterial == fixture.materialNamed("Paint").id);
        CHECK(mesh.slots[1].defaultMaterial == fixture.materialNamed("Glass").id);
        CHECK(fixture.exists("assets/client/Front/Materials/Paint.tematerial"));
    }

    SECTION("a material that no mesh uses is not written") {
        fixture.importFile("Front.gltf");
        const std::vector<AssetRecord> materials = fixture.recordsOf(TechEngine::MATERIAL_TYPE_TAG);
        CHECK(materials.size() == 2);
        CHECK_FALSE(fixture.exists("assets/client/Front/Materials/Unused.tematerial"));
        for (const AssetRecord& record: materials) {
            const Material material = fixture.readMaterial(record.path);
            CHECK(material.id == record.id);
            CHECK(material.importBlock.sourcePath == "assets/client/Front.gltf");
            CHECK(material.importBlock.settings == ImportSettings{});
            CHECK(record.side == TechEngine::Side::Client);
        }
        CHECK(fixture.materialNamed("Paint").importBlock.sourceIndex == 0);
        CHECK(fixture.materialNamed("Glass").importBlock.sourceIndex == 1);
    }
}

TEST_CASE("an unnamed material's file name comes from its index", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsUnnamed"};
    fixture.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
    fixture.importFile("Pair.gltf");

    const std::vector<AssetRecord> materials = fixture.recordsOf(TechEngine::MATERIAL_TYPE_TAG);
    REQUIRE(materials.size() == 1);
    CHECK(materials[0].path == "project://assets/client/Pair/Materials/Material 0.tematerial");
    const Material material = fixture.readMaterial(materials[0].path);
    CHECK(material.importBlock.sourceIndex == 0);
    CHECK(material.importBlock.sourceName.empty());
    Material defaults;
    defaults.id = material.id;
    defaults.importBlock = material.importBlock;
    CHECK(material == defaults);
    for (const AssetRecord& record: fixture.recordsOf(TechEngine::MESH_TYPE_TAG)) {
        CHECK(fixture.readMesh(record.path).slots[0].defaultMaterial == material.id);
    }
    CHECK_FALSE(fixture.exists("assets/client/Pair/Textures"));
}

TEST_CASE("a glTF material's factors survive the import", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsFactors"};
    fixture.scratch.writeFile(
        "assets/client/Kit.gltf",
        materialsGltf(
            2,
            R"("extensionsUsed":["KHR_materials_unlit","KHR_materials_emissive_strength"],)"
            R"("materials":[{"name":"Metal","pbrMetallicRoughness":{"baseColorFactor":[0.5,0.25,0.125,0.75],"metallicFactor":0.3,"roughnessFactor":0.6},)"
            R"("emissiveFactor":[0.5,0.25,1.0],"extensions":{"KHR_materials_emissive_strength":{"emissiveStrength":4.0}},)"
            R"("alphaMode":"MASK","alphaCutoff":0.25,"doubleSided":true},)"
            R"({"name":"Glow","alphaMode":"BLEND","extensions":{"KHR_materials_unlit":{}}}])"));
    fixture.importFile("Kit.gltf");

    const Material metal = fixture.materialNamed("Metal");
    CHECK(metal.shadingModel == MaterialShadingModel::Lit);
    CHECK(metal.baseColorFactor == Vec4(0.5f, 0.25f, 0.125f, 0.75f));
    CHECK(metal.metallicFactor == Catch::Approx(0.3f).margin(TOLERANCE));
    CHECK(metal.roughnessFactor == Catch::Approx(0.6f).margin(TOLERANCE));
    CHECK(metal.emissiveFactor == Vec3(2.0f, 1.0f, 4.0f));
    CHECK(metal.alphaMode == MaterialAlphaMode::Cutout);
    CHECK(metal.alphaCutoff == Catch::Approx(0.25f).margin(TOLERANCE));
    CHECK(metal.doubleSided);
    CHECK(metal.occlusionStrength == 1.0f);
    CHECK(metal.normalScale == 1.0f);

    const Material glow = fixture.materialNamed("Glow");
    CHECK(glow.shadingModel == MaterialShadingModel::Unlit);
    CHECK(glow.alphaMode == MaterialAlphaMode::Blended);
    CHECK_FALSE(glow.doubleSided);
    CHECK(glow.baseColorFactor == Vec4(1.0f));
    CHECK(glow.metallicFactor == 1.0f);
    CHECK(glow.roughnessFactor == 1.0f);
    CHECK(glow.emissiveFactor == Vec3(0.0f));
    CHECK(glow.alphaCutoff == Catch::Approx(0.5f));
}

TEST_CASE("a texture's scale and strength reach the material", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsTextureFactors"};
    fixture.scratch.writeFile("assets/client/Textures/flat.png", TechEngineTests::solidPng(1, 1, {128, 128, 255, 255}));
    fixture.scratch.writeFile(
        "assets/client/Kit.gltf",
        materialsGltf(
            1,
            R"("materials":[{"name":"Panel","normalTexture":{"index":0,"scale":0.5},"occlusionTexture":{"index":0,"strength":0.25}}],)"
            R"("textures":[{"source":0}],"images":[{"uri":"Textures/flat.png"}])"));
    fixture.importFile("Kit.gltf");

    const Material panel = fixture.materialNamed("Panel");
    CHECK(panel.normalScale == Catch::Approx(0.5f));
    CHECK(panel.occlusionStrength == Catch::Approx(0.25f));
    CHECK(panel.normalTexture.id.valid());
    CHECK(panel.occlusionRoughnessMetallicTexture.id == fixture.textureKeyed("Textures/flat.png+#orm#repeat-linear").id);
}

TEST_CASE("an image that cannot be read is logged, its input stays empty and the import goes on", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsMissingImage"};
    fixture.scratch.writeFile("assets/client/Textures/broken.png", "not a png");
    fixture.scratch.writeFile(
        "assets/client/Wall.gltf",
        materialsGltf(
            2,
            R"("materials":[{"name":"Gone","pbrMetallicRoughness":{"baseColorTexture":{"index":0}}},)"
            R"({"name":"Broken","normalTexture":{"index":1},"emissiveTexture":{"index":2}}],)"
            R"("textures":[{"source":0},{"source":1},{"source":2}],)"
            R"("images":[{"uri":"Textures/missing.png"},{"uri":"Textures/broken.png"},{"uri":"../../../outside.png"}])"));
    fixture.importFile("Wall.gltf");

    CHECK(fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG).empty());
    CHECK(fixture.recordsOf(TechEngine::MATERIAL_TYPE_TAG).size() == 2);
    CHECK_FALSE(fixture.materialNamed("Gone").baseColorTexture.id.valid());
    CHECK_FALSE(fixture.materialNamed("Broken").normalTexture.id.valid());
    CHECK_FALSE(fixture.materialNamed("Broken").emissiveTexture.id.valid());
    CHECK_FALSE(fixture.exists("assets/client/Wall/Textures"));
}

TEST_CASE("an OBJ material maps its diffuse colour, opacity and emissive, and is not metal", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsObj"};
    fixture.scratch.writeFile("assets/client/Quad.mtl", "newmtl Red\nKd 1 0 0\nd 0.5\nKe 0.1 0.2 0.3\n");
    fixture.scratch.writeFile("assets/client/Quad.obj", "mtllib Quad.mtl\no Quad\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nusemtl Red\nf 1 2 3 4\n");
    fixture.importFile("Quad.obj");

    REQUIRE(fixture.recordsOf(TechEngine::MATERIAL_TYPE_TAG).size() == 1);
    const Material red = fixture.materialNamed("Red");
    CHECK(red.baseColorFactor == Vec4(1.0f, 0.0f, 0.0f, 0.5f));
    CHECK(red.metallicFactor == 0.0f);
    CHECK(red.roughnessFactor == 1.0f);
    CHECK(red.emissiveFactor.x == Catch::Approx(0.1f));
    CHECK(red.emissiveFactor.y == Catch::Approx(0.2f));
    CHECK(red.emissiveFactor.z == Catch::Approx(0.3f));
    CHECK(red.alphaMode == MaterialAlphaMode::Opaque);
}

TEST_CASE("an OBJ material's absolute image paths are looked for by name beside the source, and keyed by that name", "[editor][assets][import][materials]") {
    MaterialImportFixture fixture{"materialsAbsolutePaths"};
    fixture.scratch.writeFile("assets/client/wall.png", TechEngineTests::solidPng(2, 2, {200, 100, 50, 255}));
    fixture.scratch.writeFile("assets/client/glow.png", TechEngineTests::solidPng(2, 2, {10, 20, 30, 255}));
    fixture.scratch.writeFile("assets/client/Quad.mtl", "newmtl Brick\nKd 1 1 1\nmap_Kd C:\\art\\wall.png\nmap_Ke /home/artist/maps/glow.png\n");
    fixture.scratch.writeFile("assets/client/Quad.obj", "mtllib Quad.mtl\no Quad\nv 0 0 0\nv 1 0 0\nv 1 1 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nusemtl Brick\nf 1/1 2/2 3/3 4/4\n");
    fixture.importFile("Quad.obj");

    REQUIRE(fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG).size() == 2);
    std::vector<std::string> keys;
    for (const AssetRecord& record: fixture.recordsOf(TechEngine::TEXTURE_TYPE_TAG)) {
        keys.push_back(fixture.readTexture(record.path).importBlock.sourceName);
    }
    std::ranges::sort(keys);
    REQUIRE(keys.size() == 2);
    CHECK(keys[0].starts_with("glow.png#emissive#"));
    CHECK(keys[1].starts_with("wall.png#basecolor#"));

    const Material brick = fixture.materialNamed("Brick");
    CHECK(brick.baseColorTexture.id == fixture.textureKeyed(keys[1]).id);
    CHECK(brick.emissiveTexture.id == fixture.textureKeyed(keys[0]).id);
    CHECK(fixture.exists("assets/client/Quad/Textures/wall_basecolor.tetexture"));
}
