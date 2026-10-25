#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
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

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
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
using TechEngine::ResourceSlotId;
using TechEngine::ResourceState;
using TechEngine::StringId;
using TechEngine::Writer;

static constexpr BlobKind CACHE_RESOURCE_KIND{0x43525443, 1};
static constexpr BlobKind GATED_RESOURCE_KIND{0x47525443, 1};

static constexpr StringId CACHE_RESOURCE_TYPE_TAG{"Tests.CacheResource"};
static constexpr StringId CACHE_LATE_TYPE_TAG{"Tests.CacheLateResource"};
static constexpr StringId GATED_RESOURCE_TYPE_TAG{"Tests.GatedResource"};

static constexpr std::chrono::seconds GATE_TIMEOUT{5};

struct CacheResource {
    std::string label;
};

struct CacheLateResource {
    std::string label;
};

struct GatedResource {
    std::string label;
};

static std::error_code readTestResource(const BlobKind& kind, const std::span<const std::byte> bytes, std::string& label, std::vector<ResourceId>& dependencies) {
    Reader reader{bytes};
    BlobHeader header;
    reader.readHeader(kind, header);
    std::uint64_t id = 0;
    reader.read(id);
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
    return {};
}

static std::error_code loadCacheResource(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
    std::string label;
    const std::error_code readError = readTestResource(CACHE_RESOURCE_KIND, bytes, label, dependencies);
    if (readError) {
        return readError;
    }
    static_cast<CacheResource*>(payload)->label = std::move(label);
    return {};
}

static std::mutex g_gateMutex;
static std::condition_variable g_gateChanged;
static std::uint32_t g_gateArrivals = 0;
static std::uint32_t g_gatePassAt = 0;
static bool g_gateOpen = false;
static std::atomic<std::uint32_t> g_gateTimeouts = 0;
static std::atomic<std::uint32_t> g_gateFinished = 0;

static std::error_code loadGatedResource(const std::span<const std::byte> bytes, void* payload, std::vector<ResourceId>& dependencies) {
    {
        std::unique_lock lock{g_gateMutex};
        g_gateArrivals++;
        g_gateChanged.notify_all();
        const bool passed = g_gateChanged.wait_for(lock, GATE_TIMEOUT, [] {
            return g_gateOpen || (g_gatePassAt != 0 && g_gateArrivals >= g_gatePassAt);
        });
        if (!passed) {
            g_gateTimeouts++;
        }
    }

    std::string label;
    const std::error_code readError = readTestResource(GATED_RESOURCE_KIND, bytes, label, dependencies);
    if (!readError) {
        static_cast<GatedResource*>(payload)->label = std::move(label);
    }
    g_gateFinished++;
    return readError;
}

// Declare it after the CacheHarness: it opens the gate when destroyed, and the cache's
// destructor waits for every gated task, so the other order never returns.
class LoadGateGuard {
public:
    explicit LoadGateGuard(const std::uint32_t passAt = 0) {
        const std::lock_guard lock{g_gateMutex};
        g_gateArrivals = 0;
        g_gatePassAt = passAt;
        g_gateOpen = false;
        g_gateTimeouts = 0;
        g_gateFinished = 0;
    }

    ~LoadGateGuard() {
        open();
    }

    LoadGateGuard(const LoadGateGuard&) = delete;

    LoadGateGuard& operator=(const LoadGateGuard&) = delete;

    void open() const {
        {
            const std::lock_guard lock{g_gateMutex};
            g_gateOpen = true;
        }
        g_gateChanged.notify_all();
    }

    bool waitForArrivals(const std::uint32_t count) const {
        std::unique_lock lock{g_gateMutex};
        return g_gateChanged.wait_for(lock, GATE_TIMEOUT, [count] {
            return g_gateArrivals >= count;
        });
    }
};

static std::vector<std::string> g_cacheLogMessages;

static void captureCacheLog(const TechEngine::LogRecord& record) {
    g_cacheLogMessages.emplace_back(record.message);
}

class CacheLogCaptureGuard {
private:
    TechEngine::Level m_previousLevel;

public:
    CacheLogCaptureGuard() : m_previousLevel(TechEngine::minLevel()) {
        g_cacheLogMessages.clear();
        TechEngine::setMinLevel(TechEngine::Level::Trace);
        (void)TechEngine::addLogSink(&captureCacheLog);
    }

    ~CacheLogCaptureGuard() {
        (void)TechEngine::removeLogSink(&captureCacheLog);
        TechEngine::setMinLevel(m_previousLevel);
        g_cacheLogMessages.clear();
    }

    CacheLogCaptureGuard(const CacheLogCaptureGuard&) = delete;

    CacheLogCaptureGuard& operator=(const CacheLogCaptureGuard&) = delete;
};

static bool logNames(const ResourceId id) {
    const std::string idText = std::format("{:#018x}", id.value());
    return std::ranges::any_of(g_cacheLogMessages, [&idText](const std::string& message) {
        return message.find(idText) != std::string::npos;
    });
}

static std::vector<std::byte> makeResourceBytes(const BlobKind& kind, const ResourceId id, const std::string_view label, const std::initializer_list<ResourceId> dependencies) {
    std::vector<std::byte> bytes;
    Writer writer{bytes};
    writer.writeHeader(kind);
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
        m_assets[id] = makeResourceBytes(CACHE_RESOURCE_KIND, id, label, dependencies);
    }

    void addGatedAsset(const ResourceId id, const std::string_view label) {
        m_assets[id] = makeResourceBytes(GATED_RESOURCE_KIND, id, label, {});
    }

    void addTruncatedAsset(const ResourceId id) {
        std::vector<std::byte> bytes = makeResourceBytes(CACHE_RESOURCE_KIND, id, "truncated", {});
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
    TechEngine::JobSystem jobs;
    CacheSource source;
    ResourceCache cache{jobs};

    CacheHarness() {
        cache.setSource(source);
        cache.registerLoader<CacheResource>(CACHE_RESOURCE_KIND, CACHE_RESOURCE_TYPE_TAG, &loadCacheResource);
        cache.registerLoader<GatedResource>(GATED_RESOURCE_KIND, GATED_RESOURCE_TYPE_TAG, &loadGatedResource);
    }

    template<typename T = CacheResource>
    ResourceHandle<T> request(const ResourceId id) {
        ResourceHandle<T> handle;
        const std::error_code requestError = cache.request(id, handle);
        REQUIRE(requestError == std::error_code{});
        return handle;
    }

    template<typename T>
    ResourceState state(const ResourceHandle<T> handle) {
        ResourceState out = ResourceState::Pending;
        REQUIRE(cache.getState(handle, out));
        return out;
    }

    template<typename T>
    std::uint32_t acquireCount(const ResourceHandle<T> handle) {
        std::uint32_t out = 0;
        REQUIRE(cache.getAcquireCount(handle, out));
        return out;
    }

    template<typename T>
    std::error_code error(const ResourceHandle<T> handle) {
        std::error_code out;
        REQUIRE(cache.getError(handle, out));
        return out;
    }

    template<typename T>
    bool exists(const ResourceHandle<T> handle) {
        ResourceState out = ResourceState::Pending;
        return cache.getState(handle, out);
    }
};

static constexpr ResourceId ROOT_ID{0x1001};
static constexpr ResourceId DEPENDENCY_ID{0x1002};
static constexpr ResourceId LEAF_ID{0x1003};
static constexpr ResourceId SECOND_ROOT_ID{0x1004};
static constexpr ResourceId UNKNOWN_ID{0x2001};
static constexpr ResourceId SERVER_ID{0x2002};
static constexpr ResourceId GATED_ID{0x3001};
static constexpr ResourceId SECOND_GATED_ID{0x3002};

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

TEST_CASE("the loads of one round run at the same time on the job system", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addGatedAsset(GATED_ID, "first");
    harness.source.addGatedAsset(SECOND_GATED_ID, "second");
    const LoadGateGuard gate{2};

    const ResourceHandle<GatedResource> first = harness.request<GatedResource>(GATED_ID);
    const ResourceHandle<GatedResource> second = harness.request<GatedResource>(SECOND_GATED_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE(g_gateTimeouts.load() == 0);
    REQUIRE(g_gateFinished.load() == 2);
    REQUIRE(harness.state(first) == ResourceState::Ready);
    REQUIRE(harness.state(second) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(first) != nullptr);
    REQUIRE(harness.cache.resolve(first)->label == "first");
    REQUIRE(harness.cache.resolve(second) != nullptr);
    REQUIRE(harness.cache.resolve(second)->label == "second");
}

TEST_CASE("a submitted load stays pending until a drain publishes it", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addGatedAsset(GATED_ID, "gated");
    const LoadGateGuard gate;
    const ResourceHandle<GatedResource> handle = harness.request<GatedResource>(GATED_ID);

    harness.cache.submitLoads();
    REQUIRE(gate.waitForArrivals(1));

    REQUIRE(harness.cache.removeCompletions() == std::error_code{});
    REQUIRE(harness.state(handle) == ResourceState::Pending);
    REQUIRE(harness.cache.resolve(handle) == nullptr);

    gate.open();
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE(g_gateArrivals == 1);
    REQUIRE(harness.state(handle) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(handle) != nullptr);
    REQUIRE(harness.cache.resolve(handle)->label == "gated");
}

TEST_CASE("a failed load keeps its error on its entry while the phase finishes every round", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    harness.source.addTruncatedAsset(SECOND_ROOT_ID);

    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    const ResourceHandle<CacheResource> broken = harness.request(SECOND_ROOT_ID);

    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Truncated);

    REQUIRE(harness.state(broken) == ResourceState::Failed);
    REQUIRE(harness.error(broken) == ReadError::Truncated);
    REQUIRE(harness.cache.resolve(broken) == nullptr);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.error(root) == std::error_code{});
    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    REQUIRE(harness.state(dependency) == ResourceState::Ready);

    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    REQUIRE(harness.state(broken) == ResourceState::Failed);
    REQUIRE(harness.error(broken) == ReadError::Truncated);
}

TEST_CASE("a stale handle reports no error", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addTruncatedAsset(ROOT_ID);
    const ResourceHandle<CacheResource> handle = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == ReadError::Truncated);

    harness.cache.release(handle);
    harness.cache.drainFrees();

    std::error_code out;
    REQUIRE_FALSE(harness.cache.getError(handle, out));
}

TEST_CASE("an entry still loading defers its free, and the drain does not wait for its task", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addGatedAsset(GATED_ID, "gated");
    const ResourceHandle<CacheResource> ready = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    const LoadGateGuard gate;
    const ResourceHandle<GatedResource> loading = harness.request<GatedResource>(GATED_ID);
    harness.cache.submitLoads();
    REQUIRE(gate.waitForArrivals(1));

    harness.cache.release(ready);
    harness.cache.release(loading);
    harness.cache.drainFrees();

    REQUIRE(g_gateFinished.load() == 0);
    REQUIRE_FALSE(harness.exists(ready));
    REQUIRE(harness.exists(loading));
    REQUIRE(harness.acquireCount(loading) == 0);

    gate.open();
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE_FALSE(harness.exists(loading));
    REQUIRE(harness.cache.resolve(loading) == nullptr);
}

TEST_CASE("an entry requested again before its deferred free is kept", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addGatedAsset(GATED_ID, "gated");
    const LoadGateGuard gate;
    const ResourceHandle<GatedResource> first = harness.request<GatedResource>(GATED_ID);
    harness.cache.submitLoads();
    REQUIRE(gate.waitForArrivals(1));

    harness.cache.release(first);
    harness.cache.drainFrees();
    const ResourceHandle<GatedResource> second = harness.request<GatedResource>(GATED_ID);

    gate.open();
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE(second == first);
    REQUIRE(harness.acquireCount(second) == 1);
    REQUIRE(harness.state(second) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(second) != nullptr);
    REQUIRE(g_gateArrivals == 1);
}

TEST_CASE("shutdown waits for loads in flight and logs every entry still acquired", "[core][resources][cache]") {
    const CacheLogCaptureGuard logs;
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "held", {});
    harness.source.addAsset(SECOND_ROOT_ID, "released", {});
    harness.source.addGatedAsset(GATED_ID, "gated");
    harness.request(ROOT_ID);
    const ResourceHandle<CacheResource> released = harness.request(SECOND_ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    harness.cache.release(released);

    const LoadGateGuard gate;
    harness.request<GatedResource>(GATED_ID);
    harness.cache.submitLoads();
    REQUIRE(gate.waitForArrivals(1));

    {
        const std::jthread opener{[&gate] {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            gate.open();
        }};
        harness.cache.shutdown();
        REQUIRE(g_gateFinished.load() == 1);
    }

    REQUIRE(g_gateTimeouts.load() == 0);
    REQUIRE(logNames(ROOT_ID));
    REQUIRE(logNames(GATED_ID));
    REQUIRE_FALSE(logNames(SECOND_ROOT_ID));
}

TEST_CASE("loader registration is rejected once the cache has closed it", "[core][resources][cache]") {
    const TechEngineTests::FatalAssertGuard guard;
    CacheHarness harness;

    harness.cache.closeLoaders();

    REQUIRE(harness.cache.loadersClosed());
    REQUIRE_THROWS_AS(harness.cache.registerLoader<CacheLateResource>(CACHE_RESOURCE_KIND, CACHE_LATE_TYPE_TAG, &loadCacheResource), TechEngineTests::AssertFired);
    REQUIRE_FALSE(TechEngine::resourceTypeTag<CacheLateResource>() == CACHE_LATE_TYPE_TAG);
}

TEST_CASE("a dependency added to a Ready entry loads in the next loading phase", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    ResourceSlotId added;

    REQUIRE(harness.cache.addDependency(root, DEPENDENCY_ID, added) == std::error_code{});

    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.state(dependency) == ResourceState::Pending);
    REQUIRE(harness.acquireCount(dependency) == 2);

    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    REQUIRE(harness.state(dependency) == ResourceState::Ready);
    REQUIRE(harness.cache.resolve(dependency) != nullptr);
    REQUIRE(harness.cache.resolve(dependency)->label == "dependency");
}

TEST_CASE("an added dependency is released with its holder, beside the ones its loader named", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    harness.source.addAsset(LEAF_ID, "leaf", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    ResourceSlotId added;
    REQUIRE(harness.cache.addDependency(root, LEAF_ID, added) == std::error_code{});
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    const ResourceHandle<CacheResource> leaf = harness.request(LEAF_ID);
    REQUIRE(harness.acquireCount(dependency) == 2);
    REQUIRE(harness.acquireCount(leaf) == 2);

    harness.cache.release(root);
    harness.cache.drainFrees();

    REQUIRE_FALSE(harness.exists(root));
    REQUIRE(harness.acquireCount(dependency) == 1);
    REQUIRE(harness.acquireCount(leaf) == 1);

    harness.cache.release(dependency);
    harness.cache.release(leaf);
    harness.cache.drainFrees();

    REQUIRE_FALSE(harness.exists(dependency));
    REQUIRE_FALSE(harness.exists(leaf));
}

TEST_CASE("adding an unknown id returns UnknownResource, holds nothing and leaves the holder Ready", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    ResourceSlotId added;

    REQUIRE(harness.cache.addDependency(root, UNKNOWN_ID, added) == ReadError::UnknownResource);

    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.error(root) == std::error_code{});
    REQUIRE(harness.cache.resolve(root) != nullptr);
    ResourceHandle<CacheResource> unknown;
    REQUIRE(harness.cache.request(UNKNOWN_ID, unknown) == ReadError::UnknownResource);

    harness.cache.release(root);
    harness.cache.drainFrees();

    REQUIRE_FALSE(harness.exists(root));
}

TEST_CASE("adding a dependency that closes a cycle returns CyclicReference and releases its acquire", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {DEPENDENCY_ID});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
    ResourceSlotId added;

    REQUIRE(harness.cache.addDependency(dependency, ROOT_ID, added) == ReadError::CyclicReference);

    REQUIRE(harness.acquireCount(root) == 1);
    REQUIRE(harness.acquireCount(dependency) == 2);
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.state(dependency) == ResourceState::Ready);
    REQUIRE(harness.error(dependency) == std::error_code{});

    harness.cache.release(root);
    harness.cache.drainFrees();

    REQUIRE_FALSE(harness.exists(root));
    REQUIRE(harness.acquireCount(dependency) == 1);
}

TEST_CASE("an entry added as its own dependency returns CyclicReference and keeps its count", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
    ResourceSlotId added;

    REQUIRE(harness.cache.addDependency(root, ROOT_ID, added) == ReadError::CyclicReference);

    REQUIRE(harness.acquireCount(root) == 1);
    REQUIRE(harness.state(root) == ResourceState::Ready);

    harness.cache.release(root);
    harness.cache.drainFrees();

    REQUIRE_FALSE(harness.exists(root));
}

TEST_CASE("adding a dependency to an entry that is not Ready is a programmer error", "[core][resources][cache]") {
    const TechEngineTests::FatalAssertGuard guard;
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    const ResourceHandle<CacheResource> pending = harness.request(ROOT_ID);
    ResourceSlotId added;

    REQUIRE_THROWS_AS(harness.cache.addDependency(pending, DEPENDENCY_ID, added), TechEngineTests::AssertFired);
}

TEST_CASE("an added dependency hands back the slot of the entry it acquired", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    harness.source.addAsset(DEPENDENCY_ID, "dependency", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    SECTION("a dependency the cache did not hold yet") {
        ResourceSlotId added;
        REQUIRE(harness.cache.addDependency(root, DEPENDENCY_ID, added) == std::error_code{});

        const ResourceHandle<CacheResource> dependency = harness.request(DEPENDENCY_ID);
        REQUIRE(added.valid());
        REQUIRE(added == dependency.slot());
    }

    SECTION("a dependency the cache already holds") {
        const ResourceHandle<CacheResource> held = harness.request(DEPENDENCY_ID);
        REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
        ResourceSlotId added;

        REQUIRE(harness.cache.addDependency(root, DEPENDENCY_ID, added) == std::error_code{});

        REQUIRE(added == held.slot());
        REQUIRE(harness.acquireCount(held) == 2);
    }
}

TEST_CASE("taking a Ready payload moves it out and leaves the entry Ready with a default payload", "[core][resources][cache]") {
    CacheHarness harness;
    harness.source.addAsset(ROOT_ID, "root", {});
    const ResourceHandle<CacheResource> root = harness.request(ROOT_ID);
    REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});

    CacheResource taken;
    REQUIRE(harness.cache.takePayload(root, taken));

    REQUIRE(taken.label == "root");
    REQUIRE(harness.state(root) == ResourceState::Ready);
    REQUIRE(harness.acquireCount(root) == 1);
    const CacheResource* left = harness.cache.resolve(root);
    REQUIRE(left != nullptr);
    REQUIRE(left->label.empty());
}

TEST_CASE("taking from an entry that is not Ready returns false and leaves out alone", "[core][resources][cache]") {
    CacheHarness harness;
    ResourceHandle<CacheResource> handle;

    SECTION("pending") {
        harness.source.addAsset(ROOT_ID, "root", {});
        handle = harness.request(ROOT_ID);
    }

    SECTION("failed") {
        harness.source.addTruncatedAsset(ROOT_ID);
        handle = harness.request(ROOT_ID);
        REQUIRE(harness.cache.runLoadingPhase() == ReadError::Truncated);
    }

    SECTION("absent for this role") {
        harness.source.addAbsentForRole(SERVER_ID);
        handle = harness.request(SERVER_ID);
    }

    SECTION("stale") {
        harness.source.addAsset(ROOT_ID, "root", {});
        handle = harness.request(ROOT_ID);
        REQUIRE(harness.cache.runLoadingPhase() == std::error_code{});
        harness.cache.release(handle);
        harness.cache.drainFrees();
    }

    CacheResource out;
    out.label = "untouched";
    REQUIRE_FALSE(harness.cache.takePayload(handle, out));
    REQUIRE(out.label == "untouched");
}
