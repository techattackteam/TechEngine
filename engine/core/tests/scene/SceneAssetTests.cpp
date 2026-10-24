#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using TechEngine::AssetAvailability;
using TechEngine::ComponentRegistry;
using TechEngine::Entity;
using TechEngine::ReadError;
using TechEngine::ResourceCache;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::Scene;
using TechEngine::SceneAsset;
using TechEngine::SceneBytes;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;

struct SceneAssetHealth {
    std::uint32_t value = 0;

    template<typename Archive>
    friend void visit(Archive& archive, SceneAssetHealth& health) {
        archive.field(health.value);
    }
};

struct SceneAssetNarrowHealth {
    std::uint16_t value = 0;

    template<typename Archive>
    friend void visit(Archive& archive, SceneAssetNarrowHealth& health) {
        archive.field(health.value);
    }
};

static constexpr ResourceId SCENE_ASSET_ID{0x5EC0FFEE12345678};

static TransformValues makeLocal(const float seed) {
    TransformValues values;
    values.position = Vec3(seed, seed * 2.0f, -seed);
    values.rotation = Transform::fromEulerDegrees(Vec3(seed * 10.0f, seed * 20.0f, seed * 30.0f));
    values.scale = Vec3(1.0f + seed, 2.0f, 0.5f + seed);
    return values;
}

static bool sameLocal(const TransformValues& left, const TransformValues& right) {
    return left.position == right.position && left.rotation == right.rotation && left.scale == right.scale;
}

struct SmallHierarchy {
    Entity root;
    Entity first;
    Entity second;
    Entity third;
    Entity grandchild;
};

// Children are created in one order and linked in another, so a writer that walks storage
// order instead of sibling order fails the round-trip.
static SmallHierarchy buildSmallHierarchy(Scene& scene) {
    SmallHierarchy hierarchy;
    hierarchy.root = scene.createEntity();
    hierarchy.third = scene.createEntity();
    hierarchy.first = scene.createEntity();
    hierarchy.grandchild = scene.createEntity();
    hierarchy.second = scene.createEntity();

    REQUIRE(scene.setParent(hierarchy.first, hierarchy.root, 0));
    REQUIRE(scene.setParent(hierarchy.second, hierarchy.root, 1));
    REQUIRE(scene.setParent(hierarchy.third, hierarchy.root, 2));
    REQUIRE(scene.setParent(hierarchy.grandchild, hierarchy.second, 0));

    REQUIRE(scene.getComponent<Transform>(hierarchy.root).setLocal(makeLocal(1.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.first).setLocal(makeLocal(2.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.second).setLocal(makeLocal(3.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.third).setLocal(makeLocal(4.0f)));
    REQUIRE(scene.getComponent<Transform>(hierarchy.grandchild).setLocal(makeLocal(5.0f)));
    return hierarchy;
}

static std::vector<std::byte> writeSmallHierarchyWithHealth(ComponentRegistry& registry) {
    Scene source(registry);
    const SmallHierarchy hierarchy = buildSmallHierarchy(source);
    source.addComponent<SceneAssetHealth>(hierarchy.first, SceneAssetHealth{77});

    std::vector<std::byte> bytes;
    SceneAsset{SCENE_ASSET_ID}.write(source, bytes);
    return bytes;
}

TEST_CASE("Transform carries payload adapters and Hierarchy does not", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);

    const TechEngine::ComponentTypeRecord* transform = registry.find(TechEngine::componentTypeId<Transform>());
    const TechEngine::ComponentTypeRecord* hierarchy = registry.find(TechEngine::componentTypeId<TechEngine::Hierarchy>());

    REQUIRE(transform != nullptr);
    REQUIRE(hierarchy != nullptr);
    CHECK(transform->writePayload != nullptr);
    CHECK(transform->readPayload != nullptr);
    CHECK(hierarchy->writePayload == nullptr);
    CHECK(hierarchy->readPayload == nullptr);
}

TEST_CASE("a small hierarchy round-trips through a scene asset", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    Scene source(writeRegistry);
    const SmallHierarchy written = buildSmallHierarchy(source);

    std::vector<std::byte> bytes;
    SceneAsset{SCENE_ASSET_ID}.write(source, bytes);
    REQUIRE(!bytes.empty());

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    Scene loaded(readRegistry);
    SceneAsset asset;
    const std::error_code readError = asset.read(bytes, loaded);

    REQUIRE(!readError);
    REQUIRE(asset.id == SCENE_ASSET_ID);

    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    REQUIRE(loaded.getChildren(children[0]).empty());
    REQUIRE(loaded.getChildren(children[2]).empty());
    const std::vector<Entity> grandchildren = loaded.getChildren(children[1]);
    REQUIRE(grandchildren.size() == 1);
    REQUIRE(loaded.getParent(grandchildren[0]) == children[1]);

    const std::array<Entity, 5> writtenEntities{written.root, written.first, written.second, written.third, written.grandchild};
    const std::array<Entity, 5> readEntities{roots[0], children[0], children[1], children[2], grandchildren[0]};
    for (std::size_t i = 0; i < writtenEntities.size(); i++) {
        const Transform& writtenTransform = source.getComponent<Transform>(writtenEntities[i]);
        const Transform& readTransform = loaded.getComponent<Transform>(readEntities[i]);
        CHECK(sameLocal(writtenTransform.getLocal(), readTransform.getLocal()));
        CHECK(writtenTransform.worldMatrix() == readTransform.worldMatrix());
    }
}

TEST_CASE("an empty scene round-trips through a scene asset", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    const Scene source(registry);

    std::vector<std::byte> bytes;
    SceneAsset{SCENE_ASSET_ID}.write(source, bytes);

    Scene loaded(registry);
    SceneAsset asset;
    const std::error_code readError = asset.read(bytes, loaded);

    REQUIRE(!readError);
    REQUIRE(asset.id == SCENE_ASSET_ID);
    REQUIRE(loaded.getRoots().empty());
}

TEST_CASE("every truncation of a scene asset fails soft and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(registry);
    REQUIRE(!bytes.empty());

    for (std::size_t length = 0; length < bytes.size(); length++) {
        INFO("truncated to " << length << " of " << bytes.size() << " bytes");
        Scene loaded(registry);
        SceneAsset asset;
        const std::error_code readError = asset.read(std::span<const std::byte>{bytes}.first(length), loaded);

        REQUIRE(readError == ReadError::Truncated);
        REQUIRE(loaded.getRoots().empty());
    }
}

TEST_CASE("a scene asset of another kind or version is rejected and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    std::vector<std::byte> bytes;
    TechEngine::Writer writer{bytes};

    SECTION("another kind's magic") {
        writer.writeHeader(TechEngine::BlobKind{TechEngine::SCENE_KIND.magic + 1, TechEngine::SCENE_KIND.version});

        REQUIRE(asset.read(bytes, loaded) == ReadError::BadMagic);
    }

    SECTION("another version") {
        writer.writeHeader(TechEngine::BlobKind{TechEngine::SCENE_KIND.magic, static_cast<std::uint16_t>(TechEngine::SCENE_KIND.version + 1)});

        REQUIRE(asset.read(bytes, loaded) == ReadError::BadVersion);
    }

    REQUIRE(loaded.getRoots().empty());
}

TEST_CASE("a block with an unregistered tag is skipped and the rest of the asset loads", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    writeRegistry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(writeRegistry);

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    Scene loaded(readRegistry);
    SceneAsset asset;
    const std::error_code readError = asset.read(bytes, loaded);

    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    CHECK(sameLocal(loaded.getComponent<Transform>(children[0]).getLocal(), makeLocal(2.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[1]).getLocal(), makeLocal(3.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[2]).getLocal(), makeLocal(4.0f)));
}

TEST_CASE("a registered block whose payload does not fill its length is skipped", "[core][scene]") {
    ComponentRegistry writeRegistry;
    TechEngineTests::registerBuiltInSceneComponents(writeRegistry);
    writeRegistry.registerComponent<SceneAssetHealth>("Tests.SceneAssetHealth");
    const std::vector<std::byte> bytes = writeSmallHierarchyWithHealth(writeRegistry);

    ComponentRegistry readRegistry;
    TechEngineTests::registerBuiltInSceneComponents(readRegistry);
    readRegistry.registerComponent<SceneAssetNarrowHealth>("Tests.SceneAssetHealth");
    Scene loaded(readRegistry);
    SceneAsset asset;
    const std::error_code readError = asset.read(bytes, loaded);

    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = loaded.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    CHECK(!loaded.hasComponent<SceneAssetNarrowHealth>(children[0]));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[0]).getLocal(), makeLocal(2.0f)));
    CHECK(sameLocal(loaded.getComponent<Transform>(children[1]).getLocal(), makeLocal(3.0f)));
}

TEST_CASE("reading into a Scene that already holds entities is a programmer error", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    const Scene source(registry);
    std::vector<std::byte> bytes;
    SceneAsset{SCENE_ASSET_ID}.write(source, bytes);

    Scene loaded(registry);
    loaded.createEntity();
    SceneAsset asset;

    const TechEngineTests::FatalAssertGuard guard;
    REQUIRE_THROWS_AS(asset.read(bytes, loaded), TechEngineTests::AssertFired);
}

struct SceneAssetRow {
    std::uint32_t authoringId = 0;
    std::uint32_t parentId = 0;
};

static constexpr std::uint32_t SCENE_ASSET_NO_PARENT = 0xFFFFFFFF;

static std::vector<std::byte> writeRows(const std::vector<SceneAssetRow>& rows) {
    std::vector<std::byte> bytes;
    TechEngine::Writer writer{bytes};
    writer.writeHeader(TechEngine::SCENE_KIND);
    writer.write(SCENE_ASSET_ID.value());
    writer.write(static_cast<std::uint32_t>(rows.size()));
    for (const SceneAssetRow& row: rows) {
        writer.write(row.authoringId);
        writer.write(row.parentId);
        writer.write(static_cast<std::uint32_t>(0));
    }
    return bytes;
}

TEST_CASE("a scene asset with a broken entity table is Malformed and leaves the Scene empty", "[core][scene]") {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    std::vector<std::byte> bytes;

    SECTION("a parent id that names no entity") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}, {1, 7}});
    }

    SECTION("a parent that comes after its child") {
        bytes = writeRows({{0, 1}, {1, SCENE_ASSET_NO_PARENT}});
    }

    SECTION("an entity that is its own parent") {
        bytes = writeRows({{0, 0}});
    }

    SECTION("an authoring id that is not its row") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}, {2, 0}});
    }

    SECTION("bytes after the last entity") {
        bytes = writeRows({{0, SCENE_ASSET_NO_PARENT}});
        bytes.push_back(std::byte{0});
    }

    REQUIRE(asset.read(bytes, loaded) == ReadError::Malformed);
    REQUIRE(loaded.getRoots().empty());
    REQUIRE(asset.id == ResourceId{});
}

class SceneLoadSource final : public TechEngine::AssetSource {
private:
    std::unordered_map<ResourceId, std::vector<std::byte>> m_assets;
    std::unordered_set<ResourceId> m_absentForRole;

public:
    void addAsset(const ResourceId id, std::vector<std::byte> bytes) {
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
        out = m_assets.at(id);
        return {};
    }
};

class SceneLoadHarness {
public:
    TechEngine::JobSystem jobs;
    SceneLoadSource source;
    ResourceCache cache{jobs};

    SceneLoadHarness() {
        cache.setSource(source);
        cache.registerLoader<SceneBytes>(TechEngine::SCENE_KIND, TechEngine::SCENE_TYPE_TAG, &SceneBytes::loadPayload);
    }

    std::uint32_t acquireCount(const ResourceHandle<SceneBytes> handle) const {
        std::uint32_t out = 0;
        REQUIRE(cache.getAcquireCount(handle, out));
        return out;
    }

    std::uint32_t countAfterRequestingAgain(const ResourceId id) {
        ResourceHandle<SceneBytes> again;
        REQUIRE(cache.request(id, again) == std::error_code{});
        return acquireCount(again);
    }
};

static std::vector<std::byte> writeSmallHierarchy(const ResourceId id = SCENE_ASSET_ID) {
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene source(registry);
    buildSmallHierarchy(source);

    std::vector<std::byte> bytes;
    SceneAsset{id}.write(source, bytes);
    return bytes;
}

static void checkSmallHierarchyShape(const Scene& scene) {
    const std::vector<Entity> roots = scene.getRoots();
    REQUIRE(roots.size() == 1);
    const std::vector<Entity> children = scene.getChildren(roots.front());
    REQUIRE(children.size() == 3);
    CHECK(scene.getChildren(children[0]).empty());
    CHECK(scene.getChildren(children[1]).size() == 1);
    CHECK(scene.getChildren(children[2]).empty());
}

TEST_CASE("the scene loader keeps the bytes as its payload and names no dependencies", "[core][scene][resources]") {
    const std::vector<std::byte> bytes = writeSmallHierarchy();
    SceneBytes payload;
    std::vector<ResourceId> dependencies;

    REQUIRE(SceneBytes::loadPayload(bytes, &payload, dependencies) == std::error_code{});

    REQUIRE(payload.bytes == bytes);
    REQUIRE(dependencies.empty());
}

TEST_CASE("load streams a scene into the live Scene and returns a Ready handle that holds it", "[core][scene][resources]") {
    SceneLoadHarness harness;
    harness.source.addAsset(SCENE_ASSET_ID, writeSmallHierarchy());
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle) == std::error_code{});

    REQUIRE(handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(harness.cache.getState(handle, state));
    REQUIRE(state == ResourceState::Ready);
    REQUIRE(harness.acquireCount(handle) == 1);
    checkSmallHierarchyShape(scene);
}

TEST_CASE("load frees the scene's bytes once they are streamed", "[core][scene][resources]") {
    SceneLoadHarness harness;
    harness.source.addAsset(SCENE_ASSET_ID, writeSmallHierarchy());
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle) == std::error_code{});

    const SceneBytes* left = harness.cache.resolve(handle);
    REQUIRE(left != nullptr);
    REQUIRE(left->bytes.empty());
    REQUIRE(left->bytes.capacity() == 0);
}

TEST_CASE("releasing a loaded scene's handle frees its entry in the next drain", "[core][scene][resources]") {
    SceneLoadHarness harness;
    harness.source.addAsset(SCENE_ASSET_ID, writeSmallHierarchy());
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;
    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle) == std::error_code{});

    harness.cache.release(handle);
    harness.cache.drainFrees();

    ResourceState state = ResourceState::Pending;
    REQUIRE_FALSE(harness.cache.getState(handle, state));
}

TEST_CASE("a scene the source does not know fails the load with UnknownResource and holds nothing", "[core][scene][resources]") {
    SceneLoadHarness harness;
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle) == ReadError::UnknownResource);

    REQUIRE_FALSE(handle.valid());
    REQUIRE(scene.getRoots().empty());
}

TEST_CASE("a scene absent for this role fails the load, leaves the Scene empty and holds nothing", "[core][scene][resources]") {
    SceneLoadHarness harness;
    harness.source.addAbsentForRole(SCENE_ASSET_ID);
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;

    const std::error_code loadError = SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle);

    REQUIRE(loadError != std::error_code{});
    REQUIRE_FALSE(handle.valid());
    REQUIRE(scene.getRoots().empty());
    REQUIRE(harness.countAfterRequestingAgain(SCENE_ASSET_ID) == 1);
}

TEST_CASE("a scene whose bytes do not parse fails the load with its read error and holds nothing", "[core][scene][resources]") {
    SceneLoadHarness harness;
    std::vector<std::byte> bytes = writeSmallHierarchy();
    bytes.resize(bytes.size() - 1);
    harness.source.addAsset(SCENE_ASSET_ID, std::move(bytes));
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene scene(registry);
    ResourceHandle<SceneBytes> handle;

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, scene, handle) == ReadError::Truncated);

    REQUIRE_FALSE(handle.valid());
    REQUIRE(scene.getRoots().empty());
    REQUIRE(harness.countAfterRequestingAgain(SCENE_ASSET_ID) == 1);
}

static constexpr ResourceId SECOND_SCENE_ASSET_ID{0x5EC0FFEE87654321};

static std::vector<std::string> g_sceneLoadLogMessages;

static void captureSceneLoadLog(const TechEngine::LogRecord& record) {
    g_sceneLoadLogMessages.emplace_back(record.message);
}

class SceneLoadLogCaptureGuard {
private:
    TechEngine::Level m_previousLevel;

public:
    SceneLoadLogCaptureGuard() : m_previousLevel(TechEngine::minLevel()) {
        g_sceneLoadLogMessages.clear();
        TechEngine::setMinLevel(TechEngine::Level::Trace);
        (void)TechEngine::addLogSink(&captureSceneLoadLog);
    }

    ~SceneLoadLogCaptureGuard() {
        (void)TechEngine::removeLogSink(&captureSceneLoadLog);
        TechEngine::setMinLevel(m_previousLevel);
        g_sceneLoadLogMessages.clear();
    }

    SceneLoadLogCaptureGuard(const SceneLoadLogCaptureGuard&) = delete;

    SceneLoadLogCaptureGuard& operator=(const SceneLoadLogCaptureGuard&) = delete;
};

static bool sceneLoadLogNames(const ResourceId id) {
    const std::string idText = std::format("{:#018x}", id.value());
    return std::ranges::any_of(g_sceneLoadLogMessages, [&idText](const std::string& message) {
        return message.find(idText) != std::string::npos;
    });
}

TEST_CASE("a scene released but not yet drained loads again from its source", "[core][scene][resources]") {
    SceneLoadHarness harness;
    harness.source.addAsset(SCENE_ASSET_ID, writeSmallHierarchy(SCENE_ASSET_ID));
    harness.source.addAsset(SECOND_SCENE_ASSET_ID, writeSmallHierarchy(SECOND_SCENE_ASSET_ID));
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene first(registry);
    Scene second(registry);
    Scene firstAgain(registry);
    ResourceHandle<SceneBytes> firstHandle;
    ResourceHandle<SceneBytes> secondHandle;
    ResourceHandle<SceneBytes> firstAgainHandle;
    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, first, firstHandle) == std::error_code{});
    REQUIRE(SceneAsset{SECOND_SCENE_ASSET_ID}.load(harness.cache, second, secondHandle) == std::error_code{});
    harness.cache.release(firstHandle);

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, firstAgain, firstAgainHandle) == std::error_code{});

    REQUIRE(firstAgainHandle.valid());
    REQUIRE(harness.acquireCount(firstAgainHandle) == 1);
    checkSmallHierarchyShape(firstAgain);
    REQUIRE(harness.acquireCount(secondHandle) == 1);
}

TEST_CASE("loading a scene that is still held warns, returns AlreadyLoaded and leaves the held handle alone", "[core][scene][resources]") {
    const SceneLoadLogCaptureGuard logs;
    SceneLoadHarness harness;
    harness.source.addAsset(SCENE_ASSET_ID, writeSmallHierarchy());
    ComponentRegistry registry;
    TechEngineTests::registerBuiltInSceneComponents(registry);
    Scene first(registry);
    Scene second(registry);
    ResourceHandle<SceneBytes> held;
    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, first, held) == std::error_code{});
    const ResourceHandle<SceneBytes> original = held;
    g_sceneLoadLogMessages.clear();

    REQUIRE(SceneAsset{SCENE_ASSET_ID}.load(harness.cache, second, held) == ReadError::AlreadyLoaded);

    REQUIRE(sceneLoadLogNames(SCENE_ASSET_ID));
    REQUIRE(held == original);
    REQUIRE(harness.acquireCount(held) == 1);
    REQUIRE(second.getRoots().empty());
    checkSmallHierarchyShape(first);

    harness.cache.release(held);
    harness.cache.drainFrees();

    ResourceState state = ResourceState::Pending;
    REQUIRE_FALSE(harness.cache.getState(held, state));
}
