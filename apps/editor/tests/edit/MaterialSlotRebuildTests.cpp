#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/AssetEditHarness.hpp>
#include <edit/MaterialSlotRebuild.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetRef;
using TechEngine::Entity;
using TechEngine::Material;
using TechEngine::MaterialSlot;
using TechEngine::Mesh;
using TechEngine::MeshRenderer;
using TechEngine::rebuildMaterialSlots;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceState;
using TechEngineTests::AssetEditHarness;
using TechEngineTests::EDIT_MATERIAL_ID;
using TechEngineTests::EDIT_MESH_ID;
using TechEngineTests::EDIT_PAINTED_MESH_ID;
using TechEngineTests::EDIT_UNKNOWN_ID;

static Entity createHoldingMesh(AssetEditHarness& edit, const std::string_view name, const ResourceId mesh) {
    const Entity entity = edit.createWithMeshRenderer(name);
    ResourceSlotId slot;
    REQUIRE(edit.cache.addDependency(edit.sceneHandle, mesh, slot) == std::error_code{});
    REQUIRE(edit.cache.runLoadingPhase() == std::error_code{});
    edit.scene.getComponent<MeshRenderer>(entity).mesh = AssetRef<Mesh>{mesh, ResourceHandle<Mesh>{slot.index, slot.generation}};
    return entity;
}

static const MeshRenderer& rendererOf(const AssetEditHarness& edit, const Entity entity) {
    return edit.scene.getComponent<MeshRenderer>(entity);
}

static void checkHeldMaterial(const AssetEditHarness& edit, const MaterialSlot& slot, const ResourceId id) {
    CHECK(slot.material.id == id);
    REQUIRE(slot.material.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(edit.cache.getState(slot.material.handle, state));
    CHECK(state == ResourceState::Ready);
}

TEST_CASE("a slot that takes a non-null default gets a handle to its Ready entry", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity entity = createHoldingMesh(edit, "Crate", EDIT_PAINTED_MESH_ID);

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{entity});

    const MeshRenderer& renderer = rendererOf(edit, entity);
    REQUIRE(renderer.slots.size() == 1);
    CHECK(renderer.slots[0].name == "Slot 0");
    checkHeldMaterial(edit, renderer.slots[0], EDIT_MATERIAL_ID);
}

TEST_CASE("a second rebuild over the same entities acquires nothing more", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity entity = createHoldingMesh(edit, "Crate", EDIT_PAINTED_MESH_ID);
    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{entity});
    const ResourceHandle<Material> handle = rendererOf(edit, entity).slots.at(0).material.handle;
    std::uint32_t acquiresBefore = 0;
    REQUIRE(edit.cache.getAcquireCount(handle, acquiresBefore));

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{entity});

    std::uint32_t acquiresAfter = 0;
    REQUIRE(edit.cache.getAcquireCount(handle, acquiresAfter));
    CHECK(acquiresAfter == acquiresBefore);
    CHECK(rendererOf(edit, entity).slots.at(0).material.handle == handle);
}

TEST_CASE("a material that cannot be held keeps its id with an empty handle, and the next entity is still rebuilt", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity lost = createHoldingMesh(edit, "Lost", EDIT_MESH_ID);
    edit.scene.getComponent<MeshRenderer>(lost).slots = {MaterialSlot{"Slot 0", AssetRef<Material>{EDIT_UNKNOWN_ID, {}}}};
    const Entity painted = createHoldingMesh(edit, "Painted", EDIT_PAINTED_MESH_ID);

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{lost, painted});

    const MeshRenderer& lostRenderer = rendererOf(edit, lost);
    REQUIRE(lostRenderer.slots.size() == 1);
    CHECK(lostRenderer.slots[0].material.id == EDIT_UNKNOWN_ID);
    CHECK_FALSE(lostRenderer.slots[0].material.handle.valid());
    const MeshRenderer& paintedRenderer = rendererOf(edit, painted);
    REQUIRE(paintedRenderer.slots.size() == 1);
    checkHeldMaterial(edit, paintedRenderer.slots[0], EDIT_MATERIAL_ID);
}

TEST_CASE("a renderer whose mesh is not Ready keeps its slot list as it was", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity entity = edit.createWithMeshRenderer("Pending");
    MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    renderer.mesh = AssetRef<Mesh>{EDIT_MESH_ID, ResourceHandle<Mesh>{}};
    const std::vector<MaterialSlot> before{MaterialSlot{"Old", {}}};
    renderer.slots = before;

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{entity});

    CHECK(rendererOf(edit, entity).slots == before);
}

TEST_CASE("an entity that is gone or has no renderer is skipped, and the rest are rebuilt", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity gone = createHoldingMesh(edit, "Gone", EDIT_PAINTED_MESH_ID);
    REQUIRE(edit.scene.destroyEntity(gone));
    const Entity bare = edit.scene.createEntity();
    REQUIRE(bare.valid());
    const Entity painted = createHoldingMesh(edit, "Painted", EDIT_PAINTED_MESH_ID);

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle, std::vector<Entity>{gone, bare, painted});

    CHECK_FALSE(edit.scene.hasComponent<MeshRenderer>(bare));
    const MeshRenderer& renderer = rendererOf(edit, painted);
    REQUIRE(renderer.slots.size() == 1);
    checkHeldMaterial(edit, renderer.slots[0], EDIT_MATERIAL_ID);
}

TEST_CASE("the whole-scene rebuild reaches a renderer below a root", "[editor][edit][assets]") {
    AssetEditHarness edit;
    const Entity root = edit.scene.createEntity();
    REQUIRE(root.valid());
    const Entity child = createHoldingMesh(edit, "Child", EDIT_PAINTED_MESH_ID);
    REQUIRE(edit.scene.setParent(child, root, 0));

    rebuildMaterialSlots(edit.scene, edit.cache, edit.sceneHandle);

    const MeshRenderer& renderer = rendererOf(edit, child);
    REQUIRE(renderer.slots.size() == 1);
    checkHeldMaterial(edit, renderer.slots[0], EDIT_MATERIAL_ID);
}
