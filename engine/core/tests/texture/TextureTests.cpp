#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/core/texture/Texture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::ImportBlock;
using TechEngine::ImportSettings;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::Texture;
using TechEngine::TextureFilter;
using TechEngine::TextureFormat;
using TechEngine::TextureSampler;
using TechEngine::TextureShape;
using TechEngine::TextureWrap;
using TechEngine::UpAxis;
using TechEngine::Writer;

static constexpr ResourceId TEXTURE_ASSET_ID{0x7465787400000001};

static Texture makeTexture(const TextureFormat format, const std::uint32_t width, const std::uint32_t height) {
    Texture texture;
    texture.id = TEXTURE_ASSET_ID;
    texture.importBlock = ImportBlock{"assets/client/Wall.gltf", 2, "Wall", ImportSettings{1.0F, UpAxis::Y}};
    texture.format = format;
    texture.width = width;
    texture.height = height;
    texture.sampler = TextureSampler{TextureWrap::Mirror, TextureWrap::Clamp, TextureFilter::Nearest};
    const std::uint32_t levels = Texture::mipCount(width, height);
    for (std::uint32_t level = 0; level < levels; level++) {
        std::vector<std::byte> mip(Texture::mipByteSize(format, width, height, level));
        for (std::size_t i = 0; i < mip.size(); i++) {
            mip[i] = static_cast<std::byte>((i * 7 + level * 31 + 1) & 0xFF);
        }
        texture.mips.push_back(std::move(mip));
    }
    return texture;
}

static std::vector<std::byte> writeTexture(const Texture& texture) {
    std::vector<std::byte> bytes;
    texture.write(bytes);
    return bytes;
}

struct RawTexture {
    std::uint8_t shape = 0;
    std::uint8_t format = 0;
    std::uint32_t width = 4;
    std::uint32_t height = 4;
    std::uint32_t mipCount = 3;
    std::uint8_t wrapU = 0;
    std::uint8_t wrapV = 0;
    std::uint8_t filter = 0;
    std::vector<std::uint32_t> mipSizes{16, 16, 16};
};

static std::vector<std::byte> writeRawTexture(const RawTexture& raw) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(TechEngine::TEXTURE_KIND);
    writer.write(TEXTURE_ASSET_ID.value());
    ImportBlock{}.write(writer);
    writer.write(raw.shape);
    writer.write(raw.format);
    writer.write(raw.width);
    writer.write(raw.height);
    writer.write(raw.mipCount);
    writer.write(raw.wrapU);
    writer.write(raw.wrapV);
    writer.write(raw.filter);
    for (const std::uint32_t size: raw.mipSizes) {
        writer.write(size);
        const std::vector<std::byte> data(size);
        writer.writeBytes(data);
    }
    return bytes;
}

class TextureLoadSource final : public TechEngine::AssetSource {
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

class TextureLoadHarness {
public:
    TechEngine::JobSystem jobs;
    TextureLoadSource source;
    ResourceCache cache{jobs};

    TextureLoadHarness() {
        cache.setSource(source);
        cache.registerLoader<Texture>(TechEngine::TEXTURE_KIND, TechEngine::TEXTURE_TYPE_TAG, &Texture::loadPayload);
    }
};

TEST_CASE("mipCount is the full chain down to 1x1 for the larger side", "[core][texture]") {
    CHECK(Texture::mipCount(1, 1) == 1);
    CHECK(Texture::mipCount(2, 1) == 2);
    CHECK(Texture::mipCount(1, 2) == 2);
    CHECK(Texture::mipCount(3, 3) == 2);
    CHECK(Texture::mipCount(4, 4) == 3);
    CHECK(Texture::mipCount(5, 3) == 3);
    CHECK(Texture::mipCount(1024, 1) == 11);
    CHECK(Texture::mipCount(1024, 1023) == 11);
    CHECK(Texture::mipCount(std::numeric_limits<std::uint32_t>::max(), 1) == 32);
    CHECK(Texture::mipCount(0, 8) == 0);
    CHECK(Texture::mipCount(8, 0) == 0);
}

TEST_CASE("mipByteSize takes whole 4x4 blocks for BC and 4 bytes a pixel for RGB9E5", "[core][texture]") {
    CHECK(Texture::mipByteSize(TextureFormat::Bc7Srgb, 8, 8, 0) == 64);
    CHECK(Texture::mipByteSize(TextureFormat::Bc7Unorm, 8, 8, 1) == 16);
    CHECK(Texture::mipByteSize(TextureFormat::Bc7Srgb, 8, 8, 3) == 16);
    CHECK(Texture::mipByteSize(TextureFormat::Bc5Unorm, 1, 1, 0) == 16);
    CHECK(Texture::mipByteSize(TextureFormat::Bc5Unorm, 5, 3, 0) == 32);
    CHECK(Texture::mipByteSize(TextureFormat::Bc5Unorm, 5, 3, 1) == 16);
    CHECK(Texture::mipByteSize(TextureFormat::Bc7Srgb, 16, 1, 0) == 64);
    CHECK(Texture::mipByteSize(TextureFormat::Rgb9E5, 8, 8, 0) == 256);
    CHECK(Texture::mipByteSize(TextureFormat::Rgb9E5, 5, 3, 1) == 8);
    CHECK(Texture::mipByteSize(TextureFormat::Rgb9E5, 5, 3, 2) == 4);
    CHECK(Texture::mipByteSize(TextureFormat::Rgb9E5, 1, 1, 0) == 4);
}

TEST_CASE("mipByteSize reports an overflowing level as the largest value", "[core][texture]") {
    constexpr std::uint32_t largest = std::numeric_limits<std::uint32_t>::max();

    CHECK(Texture::mipByteSize(TextureFormat::Bc7Srgb, largest, largest, 0) == std::numeric_limits<std::uint64_t>::max());
    CHECK(Texture::mipByteSize(TextureFormat::Rgb9E5, largest, largest, 0) == std::numeric_limits<std::uint64_t>::max());
    CHECK(Texture::mipByteSize(TextureFormat::Bc7Srgb, largest, largest, 31) == 16);
}

TEST_CASE("a BC7_SRGB texture with a full mip chain writes and reads back equal", "[core][texture]") {
    const Texture written = makeTexture(TextureFormat::Bc7Srgb, 8, 4);
    Texture loaded;

    REQUIRE(written.mips.size() == 4);
    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});

    CHECK(loaded.id == written.id);
    CHECK(loaded.importBlock == written.importBlock);
    CHECK(loaded.shape == TextureShape::Texture2D);
    CHECK(loaded.format == TextureFormat::Bc7Srgb);
    CHECK(loaded.width == 8);
    CHECK(loaded.height == 4);
    CHECK(loaded.sampler == written.sampler);
    CHECK(loaded.mips == written.mips);
    CHECK(loaded == written);
}

TEST_CASE("an RGB9E5 texture with a full mip chain writes and reads back equal", "[core][texture]") {
    const Texture written = makeTexture(TextureFormat::Rgb9E5, 5, 3);
    Texture loaded;

    REQUIRE(written.mips.size() == 3);
    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});

    CHECK(loaded.mips == written.mips);
    CHECK(loaded == written);
}

TEST_CASE("every format, wrap and filter round-trips", "[core][texture]") {
    Texture written = makeTexture(TextureFormat::Bc7Srgb, 4, 4);

    SECTION("BC7_UNORM") {
        written = makeTexture(TextureFormat::Bc7Unorm, 4, 4);
    }

    SECTION("BC5_UNORM") {
        written = makeTexture(TextureFormat::Bc5Unorm, 4, 4);
    }

    SECTION("repeat on both axes with linear filtering") {
        written.sampler = TextureSampler{TextureWrap::Repeat, TextureWrap::Repeat, TextureFilter::Linear};
    }

    SECTION("clamp on U and mirror on V") {
        written.sampler = TextureSampler{TextureWrap::Clamp, TextureWrap::Mirror, TextureFilter::Linear};
    }

    Texture loaded;
    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});
    CHECK(loaded == written);
}

TEST_CASE("a 1x1 texture holds one whole block in one mip", "[core][texture]") {
    const Texture written = makeTexture(TextureFormat::Bc5Unorm, 1, 1);
    Texture loaded;

    REQUIRE(written.mips.size() == 1);
    REQUIRE(written.mips[0].size() == 16);
    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});

    CHECK(loaded == written);
}

TEST_CASE("a non-square texture keeps its chain down to 1x1 on the longer side", "[core][texture]") {
    const Texture written = makeTexture(TextureFormat::Rgb9E5, 16, 1);
    Texture loaded;

    REQUIRE(written.mips.size() == 5);
    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});

    CHECK(loaded == written);
}

TEST_CASE("read replaces a texture that already holds data", "[core][texture]") {
    const Texture written = makeTexture(TextureFormat::Rgb9E5, 2, 2);
    Texture loaded = makeTexture(TextureFormat::Bc7Srgb, 8, 8);

    REQUIRE(loaded.read(writeTexture(written)) == std::error_code{});

    CHECK(loaded == written);
}

TEST_CASE("read refuses a mip count that is not the full chain", "[core][texture]") {
    Texture written = makeTexture(TextureFormat::Bc7Srgb, 8, 8);

    SECTION("a missing last level") {
        written.mips.pop_back();
    }

    SECTION("only the top level") {
        written.mips.resize(1);
    }

    SECTION("a level past 1x1") {
        written.mips.push_back(std::vector<std::byte>(16));
    }

    SECTION("no levels at all") {
        written.mips.clear();
    }

    Texture loaded;
    REQUIRE(loaded.read(writeTexture(written)) == ReadError::Malformed);
    CHECK(loaded == Texture{});
}

TEST_CASE("read refuses a mip whose byte count does not match its format and size", "[core][texture]") {
    Texture written = makeTexture(TextureFormat::Bc7Srgb, 8, 8);

    SECTION("a top level one byte short") {
        written.mips[0].pop_back();
    }

    SECTION("a top level one byte long") {
        written.mips[0].push_back(std::byte{0});
    }

    SECTION("a small level stored below one whole block") {
        written.mips[3].resize(4);
    }

    SECTION("a level empty") {
        written.mips[1].clear();
    }

    SECTION("an RGB9E5 level sized as BC data") {
        written = makeTexture(TextureFormat::Rgb9E5, 8, 8);
        written.format = TextureFormat::Bc7Srgb;
    }

    Texture loaded;
    REQUIRE(loaded.read(writeTexture(written)) == ReadError::Malformed);
    CHECK(loaded == Texture{});
}

TEST_CASE("read refuses values a reader does not know", "[core][texture]") {
    RawTexture raw;

    SECTION("a shape other than 2D") {
        raw.shape = 1;
    }

    SECTION("an unknown format") {
        raw.format = 4;
    }

    SECTION("a format of 255") {
        raw.format = 255;
    }

    SECTION("an unknown wrap on U") {
        raw.wrapU = 3;
    }

    SECTION("an unknown wrap on V") {
        raw.wrapV = 3;
    }

    SECTION("an unknown filter") {
        raw.filter = 2;
    }

    SECTION("a zero width") {
        raw.width = 0;
    }

    SECTION("a zero height") {
        raw.height = 0;
    }

    Texture loaded;
    REQUIRE(loaded.read(writeRawTexture(raw)) == ReadError::Malformed);
    CHECK(loaded == Texture{});
}

TEST_CASE("read accepts the raw writer's valid texture, so the refusals above test one field each", "[core][texture]") {
    Texture loaded;

    REQUIRE(loaded.read(writeRawTexture(RawTexture{})) == std::error_code{});

    CHECK(loaded.width == 4);
    CHECK(loaded.mips.size() == 3);
}

TEST_CASE("read refuses a huge size before it allocates", "[core][texture]") {
    RawTexture raw;
    raw.width = std::numeric_limits<std::uint32_t>::max();
    raw.height = std::numeric_limits<std::uint32_t>::max();
    raw.mipCount = 32;
    raw.mipSizes = {16};

    Texture loaded;
    REQUIRE(loaded.read(writeRawTexture(raw)) == ReadError::Malformed);
    CHECK(loaded == Texture{});
}

TEST_CASE("read rejects bytes that break the container and leaves the texture untouched", "[core][texture]") {
    std::vector<std::byte> bytes;
    std::error_code expected;

    SECTION("bytes after the last mip") {
        bytes = writeTexture(makeTexture(TextureFormat::Bc7Srgb, 8, 8));
        bytes.push_back(std::byte{0});
        expected = ReadError::Malformed;
    }

    SECTION("bytes cut inside the last mip") {
        bytes = writeTexture(makeTexture(TextureFormat::Bc7Srgb, 8, 8));
        bytes.resize(bytes.size() - 1);
        expected = ReadError::Truncated;
    }

    SECTION("bytes cut before the last mip's byte count") {
        bytes = writeTexture(makeTexture(TextureFormat::Rgb9E5, 4, 4));
        bytes.resize(bytes.size() - 4 - 4);
        expected = ReadError::Truncated;
    }

    SECTION("bytes cut inside the sampler") {
        std::vector<std::byte> full = writeTexture(makeTexture(TextureFormat::Bc7Srgb, 1, 1));
        full.resize(full.size() - 16 - sizeof(std::uint32_t) - 1);
        bytes = std::move(full);
        expected = ReadError::Truncated;
    }

    SECTION("a version 0 texture") {
        std::vector<std::byte> older;
        Writer writer{older};
        writer.writeHeader(TechEngine::BlobKind{TechEngine::TEXTURE_KIND.magic, 0});
        writer.write(TEXTURE_ASSET_ID.value());
        bytes = std::move(older);
        expected = ReadError::BadVersion;
    }

    SECTION("a newer texture version") {
        std::vector<std::byte> newer;
        Writer writer{newer};
        writer.writeHeader(TechEngine::BlobKind{TechEngine::TEXTURE_KIND.magic, static_cast<std::uint16_t>(TechEngine::TEXTURE_KIND.version + 1)});
        writer.write(TEXTURE_ASSET_ID.value());
        bytes = std::move(newer);
        expected = ReadError::BadVersion;
    }

    SECTION("another asset kind's magic") {
        std::vector<std::byte> scene;
        Writer writer{scene};
        writer.writeHeader(TechEngine::SCENE_KIND);
        writer.write(TEXTURE_ASSET_ID.value());
        bytes = std::move(scene);
        expected = ReadError::BadMagic;
    }

    Texture loaded;
    REQUIRE(loaded.read(bytes) == expected);
    CHECK(loaded == Texture{});
}

TEST_CASE("the texture loader reads the payload and names no dependencies", "[core][texture][resources]") {
    const Texture written = makeTexture(TextureFormat::Bc7Unorm, 8, 8);
    Texture payload;
    std::vector<ResourceId> dependencies{ResourceId{0x7465787400000003}};

    REQUIRE(Texture::loadPayload(writeTexture(written), &payload, dependencies) == std::error_code{});

    CHECK(payload == written);
    CHECK(dependencies.empty());
}

TEST_CASE("the texture loader names no dependencies when its bytes are refused", "[core][texture][resources]") {
    Texture written = makeTexture(TextureFormat::Bc7Unorm, 8, 8);
    written.mips.pop_back();
    Texture payload;
    std::vector<ResourceId> dependencies{ResourceId{0x7465787400000003}};

    REQUIRE(Texture::loadPayload(writeTexture(written), &payload, dependencies) == ReadError::Malformed);

    CHECK(dependencies.empty());
}

TEST_CASE("a cache with the texture loader registered loads a written texture back", "[core][texture][resources]") {
    TextureLoadHarness harness;
    const Texture written = makeTexture(TextureFormat::Rgb9E5, 4, 2);
    harness.source.addAsset(TEXTURE_ASSET_ID, writeTexture(written));
    ResourceHandle<Texture> handle;

    REQUIRE(harness.cache.request(TEXTURE_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    REQUIRE(state == ResourceState::Ready);
    const Texture* texture = harness.cache.resolve(handle);
    REQUIRE(texture != nullptr);
    CHECK(*texture == written);

    harness.cache.release(handle);
}

TEST_CASE("a cache fails the entry of a texture whose bytes are malformed", "[core][texture][resources]") {
    TextureLoadHarness harness;
    Texture malformed = makeTexture(TextureFormat::Bc5Unorm, 8, 8);
    malformed.mips[0].pop_back();
    harness.source.addAsset(TEXTURE_ASSET_ID, writeTexture(malformed));
    ResourceHandle<Texture> handle;

    REQUIRE(harness.cache.request(TEXTURE_ASSET_ID, handle) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Malformed);

    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    CHECK(state == ResourceState::Failed);
    CHECK(harness.cache.resolve(handle) == nullptr);

    harness.cache.release(handle);
}
