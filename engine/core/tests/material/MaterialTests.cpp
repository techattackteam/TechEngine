#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::AssetRef;
using TechEngine::ImportBlock;
using TechEngine::ImportSettings;
using TechEngine::Material;
using TechEngine::MaterialAlphaMode;
using TechEngine::MaterialShadingModel;
using TechEngine::MaterialSurface;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::Texture;
using TechEngine::TextureFormat;
using TechEngine::UpAxis;
using TechEngine::Vec3;
using TechEngine::Vec4;
using TechEngine::Writer;

static constexpr ResourceId MATERIAL_ASSET_ID{0x6D61746500000001};
static constexpr ResourceId BASE_COLOR_ID{0x7465787400000011};
static constexpr ResourceId ORM_ID{0x7465787400000012};
static constexpr ResourceId NORMAL_ID{0x7465787400000013};
static constexpr ResourceId EMISSIVE_ID{0x7465787400000014};

static AssetRef<Texture> textureRef(const ResourceId id) {
    return AssetRef<Texture>{id, ResourceHandle<Texture>{}};
}

static Material makeTexturedMaterial() {
    Material material;
    material.id = MATERIAL_ASSET_ID;
    material.importBlock = ImportBlock{"assets/client/Sponza.gltf", 7, "Bricks", ImportSettings{1.0f, UpAxis::Y}};
    material.shadingModel = MaterialShadingModel::Unlit;
    material.alphaMode = MaterialAlphaMode::Cutout;
    material.alphaCutoff = 0.25f;
    material.doubleSided = true;
    material.baseColorFactor = Vec4(0.5f, 0.25f, 0.125f, 0.75f);
    material.baseColorTexture = textureRef(BASE_COLOR_ID);
    material.occlusionStrength = 0.5f;
    material.roughnessFactor = 0.375f;
    material.metallicFactor = 0.0f;
    material.occlusionRoughnessMetallicTexture = textureRef(ORM_ID);
    material.normalScale = 2.0f;
    material.normalTexture = textureRef(NORMAL_ID);
    material.emissiveFactor = Vec3(4.0f, 2.0f, 1.0f);
    material.emissiveTexture = textureRef(EMISSIVE_ID);
    return material;
}

static std::vector<std::byte> writeMaterial(const Material& material) {
    std::vector<std::byte> bytes;
    material.write(bytes);
    return bytes;
}

struct RawMaterial {
    std::uint8_t surface = 0;
    std::uint8_t shadingModel = 0;
    std::uint8_t alphaMode = 0;
    std::uint8_t doubleSided = 0;
};

static std::vector<std::byte> writeRawMaterial(const RawMaterial& raw) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(TechEngine::MATERIAL_KIND);
    writer.write(MATERIAL_ASSET_ID.value());
    ImportBlock{}.write(writer);
    writer.write(raw.surface);
    writer.write(raw.shadingModel);
    writer.write(raw.alphaMode);
    writer.write(0.5f);
    writer.write(raw.doubleSided);
    for (int i = 0; i < 4; i++) {
        writer.write(1.0f);
    }
    writer.write(std::uint64_t{0});
    for (int i = 0; i < 3; i++) {
        writer.write(1.0f);
    }
    writer.write(std::uint64_t{0});
    writer.write(1.0f);
    writer.write(std::uint64_t{0});
    for (int i = 0; i < 3; i++) {
        writer.write(0.0f);
    }
    writer.write(std::uint64_t{0});
    return bytes;
}

class MaterialLoadSource final : public TechEngine::AssetSource {
private:
    std::unordered_map<ResourceId, std::vector<std::byte>> m_assets;

public:
    void addAsset(const ResourceId id, std::vector<std::byte> bytes) {
        m_assets[id] = std::move(bytes);
    }

    AssetAvailability find(const ResourceId id) const override {
        return m_assets.contains(id) ? AssetAvailability::Present : AssetAvailability::Unknown;
    }

    std::error_code read(const ResourceId id, std::vector<std::byte>& out) const override {
        out = m_assets.at(id);
        return {};
    }
};

class MaterialLoadHarness {
public:
    TechEngine::JobSystem jobs;
    MaterialLoadSource source;
    ResourceCache cache{jobs};

    MaterialLoadHarness() {
        cache.setSource(source);
        cache.registerLoader<Material>(TechEngine::MATERIAL_KIND, TechEngine::MATERIAL_TYPE_TAG, &Material::loadPayload);
        cache.registerLoader<Texture>(TechEngine::TEXTURE_KIND, TechEngine::TEXTURE_TYPE_TAG, &Texture::loadPayload);
    }

    void addTexture(const ResourceId id) {
        Texture texture;
        texture.id = id;
        texture.format = TextureFormat::Bc7Srgb;
        texture.width = 1;
        texture.height = 1;
        texture.mips = {std::vector<std::byte>(Texture::mipByteSize(TextureFormat::Bc7Srgb, 1, 1, 0))};
        std::vector<std::byte> bytes;
        texture.write(bytes);
        source.addAsset(id, std::move(bytes));
    }
};

TEST_CASE("a material with all four textures writes and reads back equal", "[core][material]") {
    const Material written = makeTexturedMaterial();

    Material loaded;
    REQUIRE(loaded.read(writeMaterial(written)) == std::error_code{});

    CHECK(loaded == written);
}

TEST_CASE("a material with no textures reads back with null references and its factors", "[core][material]") {
    Material written;
    written.id = MATERIAL_ASSET_ID;
    written.baseColorFactor = Vec4(0.2f, 0.4f, 0.6f, 0.8f);
    written.occlusionStrength = 0.0f;
    written.roughnessFactor = 0.5f;
    written.metallicFactor = 0.25f;
    written.normalScale = 0.5f;
    written.emissiveFactor = Vec3(1.0f, 0.5f, 0.0f);

    Material loaded;
    REQUIRE(loaded.read(writeMaterial(written)) == std::error_code{});

    CHECK(loaded == written);
    CHECK(loaded.baseColorTexture == AssetRef<Texture>{});
    CHECK(loaded.occlusionRoughnessMetallicTexture == AssetRef<Texture>{});
    CHECK(loaded.normalTexture == AssetRef<Texture>{});
    CHECK(loaded.emissiveTexture == AssetRef<Texture>{});
    CHECK(loaded.baseColorFactor == Vec4(0.2f, 0.4f, 0.6f, 0.8f));
    CHECK(loaded.roughnessFactor == 0.5f);
    CHECK(loaded.metallicFactor == 0.25f);
    CHECK(loaded.emissiveFactor == Vec3(1.0f, 0.5f, 0.0f));
}

TEST_CASE("a default material round-trips with the default factors", "[core][material]") {
    Material loaded;
    loaded.roughnessFactor = 0.0f;
    REQUIRE(loaded.read(writeMaterial(Material{})) == std::error_code{});

    CHECK(loaded == Material{});
}

TEST_CASE("every shading model, alpha mode and sidedness round-trips", "[core][material]") {
    const std::vector<MaterialShadingModel> shadingModels{MaterialShadingModel::Lit, MaterialShadingModel::Unlit};
    const std::vector<MaterialAlphaMode> alphaModes{MaterialAlphaMode::Opaque, MaterialAlphaMode::Cutout, MaterialAlphaMode::Blended};
    for (const MaterialShadingModel shadingModel: shadingModels) {
        for (const MaterialAlphaMode alphaMode: alphaModes) {
            for (const bool doubleSided: {false, true}) {
                Material written = makeTexturedMaterial();
                written.shadingModel = shadingModel;
                written.alphaMode = alphaMode;
                written.doubleSided = doubleSided;

                Material loaded;
                REQUIRE(loaded.read(writeMaterial(written)) == std::error_code{});
                CHECK(loaded == written);
            }
        }
    }
}

TEST_CASE("read replaces a material that already holds data", "[core][material]") {
    Material loaded = makeTexturedMaterial();
    const Material written;

    REQUIRE(loaded.read(writeMaterial(written)) == std::error_code{});

    CHECK(loaded == written);
}

TEST_CASE("read accepts the raw writer's valid material, so the refusals below test one field each", "[core][material]") {
    Material loaded;
    REQUIRE(loaded.read(writeRawMaterial(RawMaterial{})) == std::error_code{});

    Material expected;
    expected.id = MATERIAL_ASSET_ID;
    CHECK(loaded == expected);
}

TEST_CASE("read refuses an unknown surface, shading model, alpha mode or sidedness byte", "[core][material]") {
    RawMaterial raw;

    SECTION("an unknown surface") {
        raw.surface = 1;
    }

    SECTION("an unknown shading model") {
        raw.shadingModel = static_cast<std::uint8_t>(MaterialShadingModel::Unlit) + 1;
    }

    SECTION("an unknown alpha mode") {
        raw.alphaMode = static_cast<std::uint8_t>(MaterialAlphaMode::Blended) + 1;
    }

    SECTION("the largest alpha mode byte") {
        raw.alphaMode = 0xFF;
    }

    SECTION("a double-sided byte other than 0 or 1") {
        raw.doubleSided = 2;
    }

    Material loaded = makeTexturedMaterial();
    const Material before = loaded;
    REQUIRE(loaded.read(writeRawMaterial(raw)) == ReadError::Malformed);
    CHECK(loaded == before);
}

TEST_CASE("read rejects bytes that break the container and leaves the material untouched", "[core][material]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("bytes after the emissive texture") {
        bytes = writeMaterial(makeTexturedMaterial());
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the emissive texture") {
        bytes = writeMaterial(makeTexturedMaterial());
        bytes.resize(bytes.size() - 1);
        expected = ReadError::Truncated;
    }

    SECTION("bytes cut right after the import block") {
        std::vector<std::byte> full = writeMaterial(makeTexturedMaterial());
        std::vector<std::byte> prefix;
        Writer writer{prefix};
        writer.writeHeader(TechEngine::MATERIAL_KIND);
        writer.write(MATERIAL_ASSET_ID.value());
        makeTexturedMaterial().importBlock.write(writer);
        full.resize(prefix.size());
        bytes = std::move(full);
        expected = ReadError::Truncated;
    }

    SECTION("a version 0 material") {
        std::vector<std::byte> older;
        Writer writer{older};
        writer.writeHeader(TechEngine::BlobKind{TechEngine::MATERIAL_KIND.magic, 0});
        writer.write(MATERIAL_ASSET_ID.value());
        bytes = std::move(older);
        expected = ReadError::BadVersion;
    }

    SECTION("a newer material version") {
        std::vector<std::byte> newer;
        Writer writer{newer};
        writer.writeHeader(TechEngine::BlobKind{TechEngine::MATERIAL_KIND.magic, static_cast<std::uint16_t>(TechEngine::MATERIAL_KIND.version + 1)});
        writer.write(MATERIAL_ASSET_ID.value());
        bytes = std::move(newer);
        expected = ReadError::BadVersion;
    }

    SECTION("another asset kind's magic") {
        std::vector<std::byte> texture;
        Writer writer{texture};
        writer.writeHeader(TechEngine::TEXTURE_KIND);
        writer.write(MATERIAL_ASSET_ID.value());
        bytes = std::move(texture);
        expected = ReadError::BadMagic;
    }

    SECTION("no bytes at all") {
        expected = ReadError::Truncated;
    }

    Material loaded;
    REQUIRE(loaded.read(bytes) == expected);
    CHECK(loaded == Material{});
}

TEST_CASE("the material loader lists every non-null texture in input order", "[core][material][resources]") {
    Material payload;
    std::vector<ResourceId> dependencies{ResourceId{0x7465787400000099}};

    REQUIRE(Material::loadPayload(writeMaterial(makeTexturedMaterial()), &payload, dependencies) == std::error_code{});

    CHECK(payload == makeTexturedMaterial());
    CHECK(dependencies == std::vector<ResourceId>{BASE_COLOR_ID, ORM_ID, NORMAL_ID, EMISSIVE_ID});
}

TEST_CASE("the material loader lists the non-null textures and no null one", "[core][material][resources]") {
    Material written = makeTexturedMaterial();
    written.baseColorTexture = AssetRef<Texture>{};
    written.normalTexture = AssetRef<Texture>{};
    Material payload;
    std::vector<ResourceId> dependencies;

    REQUIRE(Material::loadPayload(writeMaterial(written), &payload, dependencies) == std::error_code{});

    CHECK(dependencies == std::vector<ResourceId>{ORM_ID, EMISSIVE_ID});
}

TEST_CASE("the material loader names no dependencies when every texture is null", "[core][material][resources]") {
    Material payload;
    std::vector<ResourceId> dependencies{ResourceId{0x7465787400000099}};

    REQUIRE(Material::loadPayload(writeMaterial(Material{}), &payload, dependencies) == std::error_code{});

    CHECK(dependencies.empty());
}

TEST_CASE("the material loader lists a texture once per input that names it", "[core][material][resources]") {
    Material written = makeTexturedMaterial();
    written.emissiveTexture = textureRef(BASE_COLOR_ID);
    Material payload;
    std::vector<ResourceId> dependencies;

    REQUIRE(Material::loadPayload(writeMaterial(written), &payload, dependencies) == std::error_code{});

    CHECK(dependencies == std::vector<ResourceId>{BASE_COLOR_ID, ORM_ID, NORMAL_ID, BASE_COLOR_ID});
}

TEST_CASE("the material loader names no dependencies when its bytes are refused", "[core][material][resources]") {
    RawMaterial raw;
    raw.alphaMode = 0xFF;
    Material payload;
    std::vector<ResourceId> dependencies{ResourceId{0x7465787400000099}};

    REQUIRE(Material::loadPayload(writeRawMaterial(raw), &payload, dependencies) == ReadError::Malformed);

    CHECK(dependencies.empty());
}

TEST_CASE("a cache loads a written material and holds each of its textures", "[core][material][resources]") {
    MaterialLoadHarness harness;
    Material written = makeTexturedMaterial();
    written.normalTexture = AssetRef<Texture>{};
    written.emissiveTexture = AssetRef<Texture>{};
    harness.source.addAsset(MATERIAL_ASSET_ID, writeMaterial(written));
    harness.addTexture(BASE_COLOR_ID);
    harness.addTexture(ORM_ID);
    ResourceHandle<Material> handle;

    REQUIRE(harness.cache.request(MATERIAL_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    REQUIRE(state == ResourceState::Ready);
    const Material* material = harness.cache.resolve(handle);
    REQUIRE(material != nullptr);
    CHECK(*material == written);

    for (const ResourceId textureId: {BASE_COLOR_ID, ORM_ID}) {
        ResourceHandle<Texture> texture;
        REQUIRE(harness.cache.request(textureId, texture) == std::error_code{});
        ResourceState textureState = ResourceState::Pending;
        REQUIRE(harness.cache.getState(texture, textureState));
        CHECK(textureState == ResourceState::Ready);
        std::uint32_t acquires = 0;
        REQUIRE(harness.cache.getAcquireCount(texture, acquires));
        CHECK(acquires == 2);
        harness.cache.release(texture);
    }

    harness.cache.release(handle);
    harness.cache.drainFrees();
}

TEST_CASE("a cache fails the entry of a material whose bytes are malformed", "[core][material][resources]") {
    MaterialLoadHarness harness;
    RawMaterial raw;
    raw.shadingModel = 0xFF;
    harness.source.addAsset(MATERIAL_ASSET_ID, writeRawMaterial(raw));
    ResourceHandle<Material> handle;

    REQUIRE(harness.cache.request(MATERIAL_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Malformed);

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    CHECK(state == ResourceState::Failed);
    CHECK(harness.cache.resolve(handle) == nullptr);

    harness.cache.release(handle);
}
