#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

using TechEngine::AssetRef;
using TechEngine::ComponentRegistry;
using TechEngine::Entity;
using TechEngine::Material;
using TechEngine::MaterialSlot;
using TechEngine::Mesh;
using TechEngine::MeshRenderer;
using TechEngine::MeshSlot;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::Scene;
using TechEngine::SceneAsset;

static constexpr ResourceId RENDERER_MESH_ID{0x4D45534852454E44};
static constexpr ResourceId BODY_DEFAULT_ID{0x4D4154000000B0D1};
static constexpr ResourceId GLASS_DEFAULT_ID{0x4D4154000000C1A5};
static constexpr ResourceId TRIM_DEFAULT_ID{0x4D41540000007212};
static constexpr ResourceId BODY_EDITED_ID{0x4D4154000000ED01};
static constexpr ResourceId GLASS_EDITED_ID{0x4D4154000000ED02};
static constexpr ResourceId RENDERER_SCENE_ID{0x5EC0FFEE0000AA01};

static Mesh makeMesh(const std::vector<MeshSlot>& slots) {
    Mesh mesh;
    mesh.id = RENDERER_MESH_ID;
    mesh.slots = slots;
    return mesh;
}

static AssetRef<Material> heldMaterial(const ResourceId id, const std::uint32_t handleIndex) {
    return AssetRef<Material>{id, ResourceHandle<Material>{handleIndex, 1}};
}

static AssetRef<Material> unheldMaterial(const ResourceId id) {
    return AssetRef<Material>{id, ResourceHandle<Material>{}};
}

static MeshRenderer makeEditedRenderer() {
    MeshRenderer renderer;
    renderer.mesh = AssetRef<Mesh>{RENDERER_MESH_ID, ResourceHandle<Mesh>{3, 1}};
    renderer.slots = {MaterialSlot{"Body", heldMaterial(BODY_EDITED_ID, 1)}, MaterialSlot{"Glass", heldMaterial(GLASS_EDITED_ID, 2)}};
    return renderer;
}

static const Mesh BODY_AND_GLASS = makeMesh({MeshSlot{"Body", 0, 3, BODY_DEFAULT_ID}, MeshSlot{"Glass", 3, 3, GLASS_DEFAULT_ID}});

TEST_CASE("a rebuild of an empty list takes each slot's default, in the mesh's order, with empty handles", "[core][mesh]") {
    MeshRenderer renderer;
    std::vector<std::string> dropped;

    renderer.rebuildSlots(BODY_AND_GLASS, dropped);

    const std::vector<MaterialSlot> expected{MaterialSlot{"Body", unheldMaterial(BODY_DEFAULT_ID)}, MaterialSlot{"Glass", unheldMaterial(GLASS_DEFAULT_ID)}};
    CHECK(renderer.slots == expected);
    CHECK(dropped.empty());
}

TEST_CASE("after the mesh's slots are reordered, a rebuild keeps each material on its named slot", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const Mesh reordered = makeMesh({MeshSlot{"Glass", 0, 3, GLASS_DEFAULT_ID}, MeshSlot{"Body", 3, 3, BODY_DEFAULT_ID}});
    std::vector<std::string> dropped;

    renderer.rebuildSlots(reordered, dropped);

    const std::vector<MaterialSlot> expected{MaterialSlot{"Glass", heldMaterial(GLASS_EDITED_ID, 2)}, MaterialSlot{"Body", heldMaterial(BODY_EDITED_ID, 1)}};
    CHECK(renderer.slots == expected);
    CHECK(dropped.empty());
}

TEST_CASE("a new slot takes its default with an empty handle, and the kept slots keep theirs", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const Mesh grown = makeMesh({MeshSlot{"Body", 0, 3, BODY_DEFAULT_ID}, MeshSlot{"Trim", 3, 3, TRIM_DEFAULT_ID}, MeshSlot{"Glass", 6, 3, GLASS_DEFAULT_ID}});
    std::vector<std::string> dropped;

    renderer.rebuildSlots(grown, dropped);

    const std::vector<MaterialSlot> expected{
        MaterialSlot{"Body", heldMaterial(BODY_EDITED_ID, 1)},
        MaterialSlot{"Trim", unheldMaterial(TRIM_DEFAULT_ID)},
        MaterialSlot{"Glass", heldMaterial(GLASS_EDITED_ID, 2)},
    };
    CHECK(renderer.slots == expected);
    CHECK(dropped.empty());
}

TEST_CASE("a removed slot is dropped and its name is reported", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const Mesh shrunk = makeMesh({MeshSlot{"Body", 0, 6, BODY_DEFAULT_ID}});
    std::vector<std::string> dropped;

    renderer.rebuildSlots(shrunk, dropped);

    const std::vector<MaterialSlot> expected{MaterialSlot{"Body", heldMaterial(BODY_EDITED_ID, 1)}};
    CHECK(renderer.slots == expected);
    CHECK(dropped == std::vector<std::string>{"Glass"});
}

TEST_CASE("a renamed slot drops the old name and takes the new name's default", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const Mesh renamed = makeMesh({MeshSlot{"Body", 0, 3, BODY_DEFAULT_ID}, MeshSlot{"Windows", 3, 3, GLASS_DEFAULT_ID}});
    std::vector<std::string> dropped;

    renderer.rebuildSlots(renamed, dropped);

    const std::vector<MaterialSlot> expected{MaterialSlot{"Body", heldMaterial(BODY_EDITED_ID, 1)}, MaterialSlot{"Windows", unheldMaterial(GLASS_DEFAULT_ID)}};
    CHECK(renderer.slots == expected);
    CHECK(dropped == std::vector<std::string>{"Glass"});
}

TEST_CASE("a kept slot whose material was cleared stays cleared instead of taking the default", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    renderer.slots[1].material = AssetRef<Material>{};
    std::vector<std::string> dropped;

    renderer.rebuildSlots(BODY_AND_GLASS, dropped);

    REQUIRE(renderer.slots.size() == 2);
    CHECK(renderer.slots[1].name == "Glass");
    CHECK(renderer.slots[1].material == AssetRef<Material>{});
}

TEST_CASE("a second rebuild over the same mesh changes nothing", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const Mesh grown = makeMesh({MeshSlot{"Glass", 0, 3, GLASS_DEFAULT_ID}, MeshSlot{"Trim", 3, 3, TRIM_DEFAULT_ID}});
    std::vector<std::string> firstDropped;
    renderer.rebuildSlots(grown, firstDropped);
    const std::vector<MaterialSlot> afterFirst = renderer.slots;

    std::vector<std::string> secondDropped;
    renderer.rebuildSlots(grown, secondDropped);

    CHECK(renderer.slots == afterFirst);
    CHECK(secondDropped.empty());
}

TEST_CASE("a rebuild leaves the renderer's mesh alone", "[core][mesh]") {
    MeshRenderer renderer = makeEditedRenderer();
    const AssetRef<Mesh> before = renderer.mesh;
    std::vector<std::string> dropped;

    renderer.rebuildSlots(BODY_AND_GLASS, dropped);

    CHECK(renderer.mesh == before);
}

static void registerRendererComponents(ComponentRegistry& registry) {
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<MeshRenderer>(MeshRenderer::tag);
}

static std::vector<std::byte> writeRendererScene(const MeshRenderer& renderer) {
    ComponentRegistry registry;
    registerRendererComponents(registry);
    Scene source(registry);
    const Entity entity = source.createEntity();
    source.addComponent<MeshRenderer>(entity, renderer);

    std::vector<std::byte> bytes;
    SceneAsset{RENDERER_SCENE_ID}.write(source, bytes);
    return bytes;
}

static const MeshRenderer& readOnlyRenderer(const std::vector<std::byte>& bytes, Scene& loaded, SceneAsset& asset) {
    const std::error_code readError = asset.read(bytes, loaded);
    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    REQUIRE(loaded.hasComponent<MeshRenderer>(roots[0]));
    return loaded.getComponent<MeshRenderer>(roots[0]);
}

static bool namesOnce(const std::vector<ResourceId>& references, const ResourceId id) {
    return std::ranges::count(references, id) == 1;
}

TEST_CASE("a scene with a two-slot renderer saves and loads its slot names and materials, with empty handles", "[core][mesh][scene]") {
    const std::vector<std::byte> bytes = writeRendererScene(makeEditedRenderer());

    ComponentRegistry registry;
    registerRendererComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    const MeshRenderer& renderer = readOnlyRenderer(bytes, loaded, asset);

    CHECK(renderer.mesh.id == RENDERER_MESH_ID);
    const std::vector<MaterialSlot> expected{MaterialSlot{"Body", unheldMaterial(BODY_EDITED_ID)}, MaterialSlot{"Glass", unheldMaterial(GLASS_EDITED_ID)}};
    CHECK(renderer.slots == expected);
}

TEST_CASE("a renderer with no slots saves and loads with none", "[core][mesh][scene]") {
    MeshRenderer empty;
    empty.mesh = AssetRef<Mesh>{RENDERER_MESH_ID, ResourceHandle<Mesh>{}};
    const std::vector<std::byte> bytes = writeRendererScene(empty);

    ComponentRegistry registry;
    registerRendererComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    const MeshRenderer& renderer = readOnlyRenderer(bytes, loaded, asset);

    CHECK(renderer.mesh.id == RENDERER_MESH_ID);
    CHECK(renderer.slots.empty());
}

TEST_CASE("the walk behind the scene's acquire set and the bake names each slot's material once, beside the mesh", "[core][mesh][scene]") {
    const std::vector<std::byte> bytes = writeRendererScene(makeEditedRenderer());

    ComponentRegistry registry;
    registerRendererComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    readOnlyRenderer(bytes, loaded, asset);
    std::vector<ResourceId> references;
    asset.collectReferences(loaded, references);

    CHECK(references.size() == 3);
    CHECK(namesOnce(references, RENDERER_MESH_ID));
    CHECK(namesOnce(references, BODY_EDITED_ID));
    CHECK(namesOnce(references, GLASS_EDITED_ID));
}

TEST_CASE("a slot with no material is named by neither the acquire set nor the bake", "[core][mesh][scene]") {
    MeshRenderer renderer = makeEditedRenderer();
    renderer.slots[1].material = AssetRef<Material>{};
    const std::vector<std::byte> bytes = writeRendererScene(renderer);

    ComponentRegistry registry;
    registerRendererComponents(registry);
    Scene loaded(registry);
    SceneAsset asset;
    readOnlyRenderer(bytes, loaded, asset);
    std::vector<ResourceId> references;
    asset.collectReferences(loaded, references);

    CHECK(references.size() == 2);
    CHECK(namesOnce(references, RENDERER_MESH_ID));
    CHECK(namesOnce(references, BODY_EDITED_ID));
}
