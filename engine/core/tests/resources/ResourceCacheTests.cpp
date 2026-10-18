#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Reader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::AssetSource;
using TechEngine::BlobHeader;
using TechEngine::BlobKind;
using TechEngine::FileError;
using TechEngine::Reader;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::StringId;
using TechEngine::Writer;

static constexpr BlobKind CACHE_RESOURCE_KIND{0x43525443, 1};

static constexpr StringId CACHE_RESOURCE_TYPE_TAG{"Tests.CacheResource"};
static constexpr StringId CACHE_LATE_TYPE_TAG{"Tests.CacheLateResource"};

struct CacheResource {
    std::string label;
};

struct CacheLateResource {
    std::string label;
};

static std::error_code loadCacheResource(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
    Reader reader{bytes};
    BlobHeader header;
    reader.readHeader(CACHE_RESOURCE_KIND, header);
    std::uint64_t id = 0;
    reader.read(id);
    std::string label;
    reader.read(label);
    std::uint32_t dependencyCount = 0;
    reader.read(dependencyCount);
    for (std::uint32_t i = 0; i < dependencyCount; i++) {
        std::uint64_t dependency = 0;
        reader.read(dependency);
        dependencies.push_back(ResourceId(dependency));
    }
    if (!reader.ok()) {
        return reader.error();
    }
    static_cast<CacheResource*>(payload)->label = std::move(label);
    return {};
}

static std::vector<std::byte> makeResourceBytes(const ResourceId id, const std::string_view label, const std::initializer_list<ResourceId> dependencies) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(CACHE_RESOURCE_KIND);
    writer.write(id.value());
    writer.write(label);
    writer.write(static_cast<std::uint32_t>(dependencies.size()));
    for (const ResourceId dependency: dependencies) {
        writer.write(dependency.value());
    }
    return bytes;
}

class CacheSource final : public AssetSource {
private:
    std::unordered_map<ResourceId, std::vector<std::byte>> m_assets;
    std::unordered_set<ResourceId> m_absentForRole;

public:
    void addAsset(const ResourceId id, const std::string_view label, const std::initializer_list<ResourceId> dependencies) {
        m_assets[id] = makeResourceBytes(id, label, dependencies);
    }

    void addTruncatedAsset(const ResourceId id) {
        std::vector<std::byte> bytes = makeResourceBytes(id, "truncated", {});
        bytes.resize(bytes.size() - 4);
        m_assets[id] = std::move(bytes);
    }

    void addAbsentForRole(const ResourceId id) {
        m_absentForRole.insert(id);
    }

    AssetAvailability find(const ResourceId id) const override {
        if (m_assets.contains(id)) {
            return AssetAvailability::Present;
        }
        if (m_absentForRole.contains(id)) {
            return AssetAvailability::AbsentForRole;
        }
        return AssetAvailability::Unknown;
    }

    std::error_code read(const ResourceId id, std::vector<std::byte>& out) const override {
        const auto asset = m_assets.find(id);
        if (asset == m_assets.end()) {
            return FileError::NotFound;
        }
        out = asset->second;
        return {};
    }
};

class CacheHarness {
public:
    CacheSource source;
    ResourceCache cache;

    CacheHarness() {
        cache.setSource(source);
        cache.registerLoader<CacheResource>(CACHE_RESOURCE_KIND, CACHE_RESOURCE_TYPE_TAG, &loadCacheResource);
    }

    ResourceHandle<CacheResource> request(const ResourceId id) {
        ResourceHandle<CacheResource> handle;
        const std::error_code requestError = cache.request(id, handle);
        REQUIRE(requestError == std::error_code{});
        return handle;
    }

    ResourceState state(const ResourceHandle<CacheResource> handle) {
        ResourceState out = ResourceState::Pending;
        REQUIRE(cache.getState(handle, out));
        return out;
    }

    std::uint32_t acquireCount(const ResourceHandle<CacheResource> handle) {
        std::uint32_t out = 0;
        REQUIRE(cache.getAcquireCount(handle, out));
        return out;
    }
};

static constexpr ResourceId ROOT_ID{0x1001};
static constexpr ResourceId DEPENDENCY_ID{0x1002};
static constexpr ResourceId LEAF_ID{0x1003};
static constexpr ResourceId SECOND_ROOT_ID{0x1004};
static constexpr ResourceId UNKNOWN_ID{0x2001};
static constexpr ResourceId SERVER_ID{0x2002};

TEST_CASE("an unknown id fails at once and creates no entry", "[core][resources][cache]") {
    CacheHarness harness;

    ResourceHandle<CacheResource> handle;
    const std::error_code requestError = harness.cache.request(UNKNOWN_ID, handle);

    REQUIRE(requestError == ReadError::UnknownResource);
    REQUIRE_FALSE(handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE_FALSE(harness.cache.getState(handle, state));
}

TEST_CASE("an id absent for this role gets an entry that never loads", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAbsentForRole(SERVER_ID);

    const ResourceHandle<CacheResource> handle = harness.request(SERVER_ID);

    REQUIRE(handle.valid());
    REQUIRE(harness.state(handle) == ResourceState::AbsentForRole);
    REQUIRE(harness.cache.resolve(handle) == nullptr);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    REQUIRE(harness.state(handle) == ResourceState::AbsentForRole);
    REQUIRE(harness.cache.resolve(handle) == nullptr);
}

TEST_CASE("a request creates a pending entry that the loading phase publishes", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});

    const ResourceHandle<CacheResource> handle = harness.request(ROOT_ID);
    REQUIRE(harness.state(handle) == ResourceState::Pending);
    REQUIRE(harness.cache.resolve(handle) == nullptr);

    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE(harness.state(handle) == ResourceState::Ready);
    const CacheResource* resource = harness.cache.resolve(handle);
    REQUIRE(resource != nullptr);
    REQUIRE(resource->label == "root");
}

TEST_CASE("a second request for the same id shares one entry and counts both acquires", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});

    const ResourceHandle<CacheResource> first = harness.request(ROOT_ID);
    const ResourceHandle<CacheResource> second = harness.request(ROOT_ID);

    REQUIRE(second == first);
    REQUIRE(harness.acquireCount(first) == 2);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    REQUIRE(harness.cache.resolve(second) == harness.cache.resolve(first));
}

TEST_CASE("a loading phase with nothing pending reports no error", "[core][resources][cache]") {
    CacheHarness harness;

    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
}

TEST_CASE("release frees an entry at zero only in the next drain", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    const ResourceHandle<CacheResource> handle = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    harness.cache.release(handle);

    REQUIRE(harness.acquireCount(handle) == 0);
    REQUIRE(harness.state(handle) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(handle) != nullptr);

    harness.cache.drainFrees();

    ResourceState state = ResourceState::Pending;
    REQUIRE_FALSE(harness.cache.getState(handle, state));
    REQUIRE(harness.cache.resolve(handle) == nullptr);
}

TEST_CASE("an entry still acquired survives a drain", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    const ResourceHandle<CacheResource> first = harness.request(ROOT_ID);
    const ResourceHandle<CacheResource> second = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    harness.cache.release(second);
    harness.cache.drainFrees();

    REQUIRE(harness.acquireCount(first) == 1);
    REQUIRE(harness.state(first) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(first) != nullptr);
}

TEST_CASE("a freed slot is reused with an advanced generation", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addAsset(SECOND_ROOT_ID, "second", {});
    const ResourceHandle<CacheResource> freed = harness.request(ROOT_ID);
    harness.cache.release(freed);
    harness.cache.drainFrees();

    const ResourceHandle<CacheResource> reused = harness.request(SECOND_ROOT_ID);

    REQUIRE(reused.index == freed.index);
    REQUIRE(reused.generation == freed.generation + 1);
    REQUIRE(harness.cache.resolve(freed) == nullptr);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    REQUIRE(harness.cache.resolve(freed) == nullptr);
    REQUIRE(harness.cache.resolve(reused) != nullptr);
}

TEST_CASE("releasing a stale handle leaves the entry that reused its slot alone", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addAsset(SECOND_ROOT_ID, "second", {});
    const ResourceHandle<CacheResource> stale = harness.request(ROOT_ID);
    harness.cache.release(stale);
    harness.cache.drainFrees();
    const ResourceHandle<CacheResource> reused = harness.request(SECOND_ROOT_ID);

    harness.cache.release(stale);
    harness.cache.drainFrees();

    REQUIRE(harness.acquireCount(reused) == 1);
    REQUIRE(harness.state(reused) == ResourceState::Pending);
}

TEST_CASE("the loading phase acquires every dependency its loader names", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});

    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.state(dependency) == ResourceState::Ready);
    REQUIRE(harness.acquireCount(root) == 1);
    REQUIRE(harness.acquireCount(dependency) == 2);
}

TEST_CASE("one loading phase resolves a dependency chain in rounds", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {LEAF_ID});
    harness.source.addAsset(LEAF_ID, "leaf", {});

    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    const ResourceHandle<CacheResource> leaf = harness.request(LEAF_ID);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.state(dependency) == ResourceState::Ready);
    REQUIRE(harness.state(leaf) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(leaf) != nullptr);
    REQUIRE(harness.cache.resolve(leaf)->label == "leaf");
}

TEST_CASE("a dependency two holders share is counted once per holder", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(SECOND_ROOT_ID, "second", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});

    harness.request(ROOT_ID);
    harness.request(SECOND_ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    REQUIRE(harness.acquireCount(dependency) == 3);
    REQUIRE(harness.cache.resolve(dependency) != nullptr);
}

TEST_CASE("releasing a holder releases its dependencies in the next drain", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);

    harness.cache.release(root);
    harness.cache.drainFrees();

    REQUIRE(harness.acquireCount(dependency) == 1);
    REQUIRE(harness.state(dependency) == ResourceState::Ready);

    harness.cache.release(dependency);
    harness.cache.drainFrees();

    ResourceState state = ResourceState::Pending;
    REQUIRE_FALSE(harness.cache.getState(dependency, state));
}

TEST_CASE("a failed loader ends the phase with its error and marks the entry failed", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addTruncatedAsset(ROOT_ID);

    const ResourceHandle<CacheResource> handle = harness.request(ROOT_ID);

    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Truncated);
    REQUIRE(harness.state(handle) == ResourceState::Failed);
    REQUIRE(harness.cache.resolve(handle) == nullptr);
}

TEST_CASE("a resource that references itself fails the phase as a cycle", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {ROOT_ID});

    const ResourceHandle<CacheResource> handle = harness.request(ROOT_ID);

    REQUIRE(harness.cache.runLoadingPhase() == ReadError::CyclicReference);
    REQUIRE(harness.state(handle) == ResourceState::Failed);
    REQUIRE(harness.cache.resolve(handle) == nullptr);
    REQUIRE(harness.acquireCount(handle) == 1);
}

TEST_CASE("a cycle between two resources fails the phase and leaves the counts balanced", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {ROOT_ID});

    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);

    REQUIRE(harness.cache.runLoadingPhase() == ReadError::CyclicReference);

    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    REQUIRE(harness.state(dependency) == ResourceState::Failed);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.acquireCount(root) == 1);
    REQUIRE(harness.acquireCount(dependency) == 2);
}

TEST_CASE("loader registration is rejected once the cache has closed it", "[core][resources][cache]") {
    const TechEngineTests::FatalAssertGuard guard;
    CacheHarness harness;

    harness.cache.closeLoaders();

    REQUIRE(harness.cache.loadersClosed());
    REQUIRE_THROWS_AS(harness.cache.registerLoader<CacheLateResource>(CACHE_RESOURCE_KIND, CACHE_LATE_TYPE_TAG, &loadCacheResource), TechEngineTests::AssertFired);
    REQUIRE_FALSE(TechEngine::resourceTypeTag<CacheLateResource>() == CACHE_LATE_TYPE_TAG);
}
