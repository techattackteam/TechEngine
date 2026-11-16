#include <TechEngine/client/render/FrameExtraction.hpp>
#include <TechEngine/client/render/FramePacket.hpp>
#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using TechEngine::AssetRef;
using TechEngine::Camera;
using TechEngine::ComponentRegistry;
using TechEngine::Entity;
using TechEngine::FramePacket;
using TechEngine::Material;
using TechEngine::MaterialSlot;
using TechEngine::Mesh;
using TechEngine::MeshRenderer;
using TechEngine::PacketInstance;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::Scene;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;

static ComponentRegistry& registerFrameComponents(ComponentRegistry& registry) {
    registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
    registry.registerComponent<Transform>(Transform::tag);
    registry.registerComponent<TechEngine::Name>(TechEngine::Name::tag);
    registry.registerComponent<MeshRenderer>(MeshRenderer::tag);
    registry.registerComponent<Camera>(Camera::tag);
    return registry;
}

static void place(Scene& scene, const Entity entity, const Vec3 position) {
    TransformValues local;
    local.position = position;
    const bool placed = scene.getComponent<Transform>(entity).setLocal(local);
    REQUIRE(placed);
}

static Entity createChild(Scene& scene, const Entity parent) {
    const Entity child = scene.createEntity();
    const bool parented = scene.setParent(child, parent, scene.getChildren(parent).size());
    REQUIRE(parented);
    return child;
}

static MeshRenderer renderer(const std::uint32_t meshIndex, const std::vector<std::uint32_t>& materialIndices) {
    MeshRenderer value;
    value.mesh = AssetRef<Mesh>{ResourceId{0x1000 + meshIndex}, ResourceHandle<Mesh>{meshIndex, 1}};
    for (const std::uint32_t index: materialIndices) {
        MaterialSlot slot;
        slot.name = "slot";
        slot.material = AssetRef<Material>{ResourceId{0x2000 + index}, ResourceHandle<Material>{index, 1}};
        value.slots.push_back(slot);
    }
    return value;
}

static const PacketInstance* findInstance(const FramePacket& packet, const Entity entity) {
    for (const PacketInstance& instance: packet.instances) {
        if (instance.entity == entity) {
            return &instance;
        }
    }
    return nullptr;
}

TEST_CASE("a packet carries each MeshRenderer's world transform, mesh and slot materials", "[client][extraction]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    const Entity parent = scene.createEntity();
    place(scene, parent, Vec3(10.0f, 0.0f, 0.0f));
    const Entity child = createChild(scene, parent);
    place(scene, child, Vec3(0.0f, 2.0f, 0.0f));
    scene.addComponent<MeshRenderer>(child, renderer(7, {3, 5}));
    const Entity other = scene.createEntity();
    scene.addComponent<MeshRenderer>(other, renderer(8, {4}));

    FramePacket packet;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);

    REQUIRE(packet.instances.size() == 2);
    const PacketInstance* placed = findInstance(packet, child);
    REQUIRE(placed != nullptr);
    CHECK(placed->world == scene.getComponent<Transform>(child).getWorldMatrix());
    CHECK(placed->world[3].x == 10.0f);
    CHECK(placed->world[3].y == 2.0f);
    CHECK(placed->mesh == ResourceHandle<Mesh>{7, 1});
    REQUIRE(placed->materialCount == 2);
    REQUIRE(placed->firstMaterial + placed->materialCount <= packet.materials.size());
    CHECK(packet.materials[placed->firstMaterial] == ResourceHandle<Material>{3, 1});
    CHECK(packet.materials[placed->firstMaterial + 1] == ResourceHandle<Material>{5, 1});

    const PacketInstance* single = findInstance(packet, other);
    REQUIRE(single != nullptr);
    REQUIRE(single->materialCount == 1);
    CHECK(packet.materials[single->firstMaterial] == ResourceHandle<Material>{4, 1});
}

TEST_CASE("an entity with no MeshRenderer is not an instance", "[client][extraction]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    const Entity root = scene.createEntity();
    createChild(scene, root);

    FramePacket packet;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    CHECK(packet.instances.empty());
    CHECK(packet.materials.empty());
}

TEST_CASE("a slot with no material is carried as a null handle in its place", "[client][extraction]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    const Entity entity = scene.createEntity();
    MeshRenderer value = renderer(2, {6, 9});
    value.slots[0].material = AssetRef<Material>{};
    scene.addComponent<MeshRenderer>(entity, value);

    FramePacket packet;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    REQUIRE(packet.instances.size() == 1);
    const PacketInstance& instance = packet.instances.front();
    REQUIRE(instance.materialCount == 2);
    CHECK_FALSE(packet.materials[instance.firstMaterial].valid());
    CHECK(packet.materials[instance.firstMaterial + 1] == ResourceHandle<Material>{9, 1});
}

TEST_CASE("a packet carries the main active camera's settings and world", "[client][extraction][camera]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    const Entity inactive = scene.createEntity();
    Camera side;
    side.main = false;
    side.verticalFieldOfView = 30.0f;
    scene.addComponent<Camera>(inactive, side);
    const Entity rig = scene.createEntity();
    place(scene, rig, Vec3(0.0f, 0.0f, 5.0f));
    const Entity eye = createChild(scene, rig);
    Camera main;
    main.verticalFieldOfView = 75.0f;
    main.nearPlane = 0.5f;
    main.farPlane = 400.0f;
    main.exposure = 2.0f;
    scene.addComponent<Camera>(eye, main);

    FramePacket packet;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    REQUIRE(packet.hasCamera);
    CHECK(packet.camera.world == scene.getComponent<Transform>(eye).getWorldMatrix());
    CHECK(packet.camera.settings.verticalFieldOfView == 75.0f);
    CHECK(packet.camera.settings.nearPlane == 0.5f);
    CHECK(packet.camera.settings.farPlane == 400.0f);
    CHECK(packet.camera.settings.exposure == 2.0f);
    CHECK_FALSE(warnedNoCamera);
}

TEST_CASE("a scene with no main camera gives a packet with no camera and sets the warn-once flag", "[client][extraction][camera]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    scene.createEntity();

    FramePacket packet;
    packet.hasCamera = true;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    CHECK_FALSE(packet.hasCamera);
    CHECK(warnedNoCamera);
}

TEST_CASE("extracting again replaces the packet's contents and keeps its capacity", "[client][extraction]") {
    ComponentRegistry registry;
    Scene scene(registerFrameComponents(registry));
    std::vector<Entity> entities;
    for (std::uint32_t i = 0; i < 32; i++) {
        const Entity entity = scene.createEntity();
        scene.addComponent<MeshRenderer>(entity, renderer(i, {i, i + 1}));
        entities.push_back(entity);
    }
    FramePacket packet;
    bool warnedNoCamera = false;
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    REQUIRE(packet.instances.size() == 32);
    REQUIRE(packet.materials.size() == 64);
    const std::size_t instanceCapacity = packet.instances.capacity();
    const std::size_t materialCapacity = packet.materials.capacity();

    for (std::size_t i = 1; i < entities.size(); i++) {
        const bool destroyed = scene.destroyEntity(entities[i]);
        REQUIRE(destroyed);
    }
    TechEngine::FrameExtraction::extractFramePacket(scene, warnedNoCamera, packet);
    REQUIRE(packet.instances.size() == 1);
    CHECK(packet.instances.front().entity == entities.front());
    CHECK(packet.materials.size() == 2);
    CHECK(packet.instances.capacity() == instanceCapacity);
    CHECK(packet.materials.capacity() == materialCapacity);
}
