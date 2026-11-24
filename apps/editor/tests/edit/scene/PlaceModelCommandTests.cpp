#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/LogCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/import/ModelImporter.hpp>
#include <assets/import/ModelTestFiles.hpp>
#include <assets/index/AssetIndex.hpp>
#include <edit/scene/AssetEditHarness.hpp>
#include <edit/scene/HierarchyCommands.hpp>
#include <edit/scene/PlaceModelCommand.hpp>
#include <edit/snapshot/EditorSnapshot.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

using Catch::Approx;
using TechEngine::AssetRecord;
using TechEngine::DestroyEntityCommand;
using TechEngine::Entity;
using TechEngine::MaterialSlot;
using TechEngine::MeshRenderer;
using TechEngine::Model;
using TechEngine::MODEL_NO_PARENT;
using TechEngine::MODEL_TYPE_TAG;
using TechEngine::ModelNode;
using TechEngine::Name;
using TechEngine::PlaceModelCommand;
using TechEngine::RenameEntityCommand;
using TechEngine::ResourceId;
using TechEngine::ResourceState;
using TechEngine::Transform;
using TechEngine::TransformValues;
using TechEngine::Vec3;
using TechEngineTests::AssetEditHarness;
using TechEngineTests::EDIT_BROKEN_MESH_ID;
using TechEngineTests::EDIT_MATERIAL_ID;
using TechEngineTests::EDIT_MESH_ID;
using TechEngineTests::EDIT_OTHER_SCENE_ID;
using TechEngineTests::EDIT_SECOND_MESH_ID;
using TechEngineTests::EDIT_SERVER_MESH_ID;
using TechEngineTests::EDIT_UNKNOWN_ID;
using TechEngineTests::ScratchDirectory;

static constexpr ResourceId KART_MODEL_ID{0x6D6F64656C000101};
static constexpr std::string_view KART_MODEL_PATH = "project://assets/client/Kart/Kart.temodel";
static constexpr std::string_view BROKEN_MODEL_PATH = "project://assets/client/Kart/Broken.temodel";
static constexpr std::string_view MISSING_MODEL_PATH = "project://assets/client/Kart/Missing.temodel";
// The harness indexes its assets under paths with no mount.
static constexpr std::string_view HARNESS_SCENE_PATH = "assets/common/Other.tescene";
static constexpr std::string_view HARNESS_SERVER_MESH_PATH = "assets/server/Collision.temesh";
static constexpr std::string_view HARNESS_MESH_PATH = "assets/client/Triangle.temesh";
static constexpr ResourceId MERGED_MESH_ID{0x3E54000000000101};
static constexpr std::string_view MERGED_MESH_PATH = "project://assets/client/Front/Front.temesh";
static constexpr std::uint64_t SPLIT_FIRST_ID = 0x5917000000000001;

class PlaceFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    AssetEditHarness edit;

    explicit PlaceFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client/Kart");
    }

    void addModel(const Model& model, const std::string_view path) {
        std::vector<std::byte> bytes;
        model.write(bytes);
        addModelBytes(model.id, bytes, path);
    }

    void addModelBytes(const ResourceId id, const std::span<const std::byte> bytes, const std::string_view path) {
        REQUIRE(files.write(path, bytes) == std::error_code{});
        indexModel(id, path);
    }

    void indexModel(const ResourceId id, const std::string_view path) {
        AssetRecord record;
        record.id = id;
        record.typeTag = MODEL_TYPE_TAG;
        record.side = TechEngine::Side::Client;
        record.path = std::string(path);
        edit.index.add(std::move(record));
    }

    void place(const std::string_view path) {
        placeUnder(path, Entity{});
    }

    void placeUnder(const std::string_view path, const Entity parent) {
        PlaceModelCommand command{edit.session, files, edit.cache, edit.sceneHandle, edit.index, std::string(path), parent};
        command.apply(edit.scene);
    }

    std::uint64_t editRevision() const {
        return edit.snapshot().editRevision;
    }
};

struct PlacedKart {
    Entity root;
    Entity body;
    Entity wheel;
};

static TransformValues makeLocal(const Vec3& position, const Vec3& eulerDegrees, const Vec3& scale) {
    TransformValues local;
    local.position = position;
    local.rotation = Transform::fromEulerDegrees(eulerDegrees);
    local.scale = scale;
    return local;
}

static const TransformValues BODY_LOCAL = makeLocal(Vec3(0.0f, 0.5f, 0.0f), Vec3(0.0f), Vec3(1.0f));
static const TransformValues WHEEL_LOCAL = makeLocal(Vec3(0.8f, -0.3f, 1.2f), Vec3(0.0f, 90.0f, 0.0f), Vec3(0.5f));

static ModelNode makeNode(const std::string& name, const std::uint32_t parent, const TransformValues& local, const ResourceId mesh) {
    ModelNode node;
    node.name = name;
    node.parent = parent;
    node.local = local;
    node.mesh = mesh;
    return node;
}

// A root with no mesh, a body under it and a wheel under the body.
static Model makeKart(const TransformValues& rootLocal, const ResourceId bodyMesh, const ResourceId wheelMesh) {
    Model model;
    model.id = KART_MODEL_ID;
    model.importBlock.sourcePath = "assets/client/Kart.gltf";
    model.nodes.push_back(makeNode("Kart", MODEL_NO_PARENT, rootLocal, ResourceId{}));
    model.nodes.push_back(makeNode("Body", 0, BODY_LOCAL, bodyMesh));
    model.nodes.push_back(makeNode("Wheel", 1, WHEEL_LOCAL, wheelMesh));
    return model;
}

static Model makeKart() {
    return makeKart(TransformValues{}, EDIT_MESH_ID, EDIT_SECOND_MESH_ID);
}

static PlacedKart walkKart(const AssetEditHarness& edit, const Entity root) {
    PlacedKart kart;
    kart.root = root;
    const std::vector<Entity> rootChildren = edit.scene.getChildren(root);
    REQUIRE(rootChildren.size() == 1);
    kart.body = rootChildren[0];
    const std::vector<Entity> bodyChildren = edit.scene.getChildren(kart.body);
    REQUIRE(bodyChildren.size() == 1);
    kart.wheel = bodyChildren[0];
    CHECK(edit.scene.getChildren(kart.wheel).empty());
    return kart;
}

static void checkLocal(AssetEditHarness& edit, const Entity entity, const TransformValues& expected) {
    const TransformValues& local = edit.scene.getComponent<Transform>(entity).getLocal();
    CHECK(local.position.x == Approx(expected.position.x));
    CHECK(local.position.y == Approx(expected.position.y));
    CHECK(local.position.z == Approx(expected.position.z));
    CHECK(local.rotation.w == Approx(expected.rotation.w));
    CHECK(local.rotation.x == Approx(expected.rotation.x));
    CHECK(local.rotation.y == Approx(expected.rotation.y));
    CHECK(local.rotation.z == Approx(expected.rotation.z));
    CHECK(local.scale.x == Approx(expected.scale.x));
    CHECK(local.scale.y == Approx(expected.scale.y));
    CHECK(local.scale.z == Approx(expected.scale.z));
}

static void checkPlacedMesh(AssetEditHarness& edit, const Entity entity, const ResourceId id) {
    REQUIRE(edit.scene.hasComponent<MeshRenderer>(entity));
    const MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    CHECK(renderer.mesh.id == id);
    REQUIRE(renderer.mesh.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(edit.cache.getState(renderer.mesh.handle, state));
    CHECK(state == ResourceState::Ready);
}

static void checkKart(AssetEditHarness& edit, const PlacedKart& kart) {
    CHECK(edit.scene.getComponent<Name>(kart.root).value == "Kart");
    CHECK(edit.scene.getComponent<Name>(kart.body).value == "Body");
    CHECK(edit.scene.getComponent<Name>(kart.wheel).value == "Wheel");
    CHECK_FALSE(edit.scene.getParent(kart.root).valid());
    CHECK(edit.scene.getParent(kart.body) == kart.root);
    CHECK(edit.scene.getParent(kart.wheel) == kart.body);
    checkLocal(edit, kart.root, TransformValues{});
    checkLocal(edit, kart.body, BODY_LOCAL);
    checkLocal(edit, kart.wheel, WHEEL_LOCAL);
    CHECK_FALSE(edit.scene.hasComponent<MeshRenderer>(kart.root));
    checkPlacedMesh(edit, kart.body, EDIT_MESH_ID);
    checkPlacedMesh(edit, kart.wheel, EDIT_SECOND_MESH_ID);
}

static void checkNothingPlaced(const PlaceFixture& fixture, const std::uint64_t revisionBefore) {
    CHECK(fixture.edit.scene.getRoots().empty());
    CHECK(fixture.edit.snapshot().hierarchy.empty());
    CHECK(fixture.editRevision() == revisionBefore);
}

TEST_CASE("placing a three-node model adds three entities with their parents and local transforms", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeThreeNodes"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK(fixture.edit.snapshot().hierarchy.size() == 3);
    checkKart(fixture.edit, walkKart(fixture.edit, roots[0]));
    CHECK(fixture.editRevision() > revisionBefore);
}

TEST_CASE("placing a model goes beside the scene's own entities and leaves them as they were", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeBesideExisting"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    const Entity existing = fixture.edit.createWithMeshRenderer("Existing");

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 2);
    const Entity placedRoot = roots[0] == existing ? roots[1] : roots[0];
    checkKart(fixture.edit, walkKart(fixture.edit, placedRoot));
    CHECK(fixture.edit.scene.getComponent<Name>(existing).value == "Existing");
    CHECK(fixture.edit.scene.getChildren(existing).empty());
    CHECK(fixture.edit.scene.getComponent<MeshRenderer>(existing).mesh.id == ResourceId{});
}

TEST_CASE("placing a model twice gives two independent trees", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeTwice"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);

    fixture.place(KART_MODEL_PATH);
    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 2);
    CHECK(fixture.edit.snapshot().hierarchy.size() == 6);
    const PlacedKart first = walkKart(fixture.edit, roots[0]);
    const PlacedKart second = walkKart(fixture.edit, roots[1]);
    checkKart(fixture.edit, first);
    checkKart(fixture.edit, second);
    CHECK(first.root != second.root);
    CHECK(first.body != second.body);
    CHECK(first.wheel != second.wheel);
}

TEST_CASE("editing one placed copy changes neither the other copy nor the model file", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeEditOneCopy"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    fixture.place(KART_MODEL_PATH);
    fixture.place(KART_MODEL_PATH);
    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 2);
    const PlacedKart edited = walkKart(fixture.edit, roots[0]);
    const PlacedKart untouched = walkKart(fixture.edit, roots[1]);
    std::vector<std::byte> fileBefore;
    REQUIRE(fixture.files.read(KART_MODEL_PATH, fileBefore) == std::error_code{});

    RenameEntityCommand rename{fixture.edit.session, {edited.body}, "Edited body"};
    rename.apply(fixture.edit.scene);
    const TransformValues movedBody = makeLocal(Vec3(5.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 30.0f), Vec3(3.0f));
    REQUIRE(fixture.edit.scene.getComponent<Transform>(edited.body).setLocal(movedBody));
    DestroyEntityCommand destroy{fixture.edit.session, {edited.wheel}};
    destroy.apply(fixture.edit.scene);

    CHECK(fixture.edit.scene.getComponent<Name>(edited.body).value == "Edited body");
    checkLocal(fixture.edit, edited.body, movedBody);
    CHECK_FALSE(fixture.edit.scene.contains(edited.wheel));
    checkKart(fixture.edit, untouched);

    std::vector<std::byte> fileAfter;
    REQUIRE(fixture.files.read(KART_MODEL_PATH, fileAfter) == std::error_code{});
    CHECK(fileAfter == fileBefore);

    fixture.place(KART_MODEL_PATH);
    const std::vector<Entity> rootsAfter = fixture.edit.scene.getRoots();
    REQUIRE(rootsAfter.size() == 3);
    Entity third;
    for (const Entity root: rootsAfter) {
        if (root != edited.root && root != untouched.root) {
            third = root;
        }
    }
    REQUIRE(third.valid());
    checkKart(fixture.edit, walkKart(fixture.edit, third));
}

TEST_CASE("a placed root keeps the transform the model stores for it", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeRootTransform"};
    const TransformValues rootLocal = makeLocal(Vec3(2.0f, 0.0f, -3.0f), Vec3(0.0f, 45.0f, 0.0f), Vec3(2.0f));
    fixture.addModel(makeKart(rootLocal, EDIT_MESH_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK_FALSE(fixture.edit.scene.getParent(roots[0]).valid());
    checkLocal(fixture.edit, roots[0], rootLocal);
    const PlacedKart kart = walkKart(fixture.edit, roots[0]);
    checkLocal(fixture.edit, kart.body, BODY_LOCAL);
    checkLocal(fixture.edit, kart.wheel, WHEEL_LOCAL);
}

TEST_CASE("a model placed on an entity becomes its first child, with the stored root transform as its local one", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeUnderEntity"};
    const TransformValues rootLocal = makeLocal(Vec3(2.0f, 0.0f, -3.0f), Vec3(0.0f, 45.0f, 0.0f), Vec3(2.0f));
    fixture.addModel(makeKart(rootLocal, EDIT_MESH_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);
    const Entity parent = fixture.edit.createWithMeshRenderer("Parent");
    // A parent away from the origin, so a placement that kept the world transform would store another local one.
    REQUIRE(fixture.edit.scene.getComponent<Transform>(parent).setLocal(makeLocal(Vec3(10.0f, 5.0f, 0.0f), Vec3(0.0f, 0.0f, 30.0f), Vec3(4.0f))));
    const Entity existingChild = fixture.edit.scene.createEntity();
    REQUIRE(fixture.edit.scene.setParent(existingChild, parent, 0));
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.placeUnder(KART_MODEL_PATH, parent);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK(roots[0] == parent);
    const std::vector<Entity> children = fixture.edit.scene.getChildren(parent);
    REQUIRE(children.size() == 2);
    CHECK(children[1] == existingChild);
    const Entity placedRoot = children[0];
    CHECK(fixture.edit.scene.getParent(placedRoot) == parent);
    CHECK(fixture.edit.scene.getComponent<Name>(placedRoot).value == "Kart");
    checkLocal(fixture.edit, placedRoot, rootLocal);
    const PlacedKart kart = walkKart(fixture.edit, placedRoot);
    CHECK(fixture.edit.scene.getParent(kart.body) == placedRoot);
    CHECK(fixture.edit.scene.getParent(kart.wheel) == kart.body);
    checkLocal(fixture.edit, kart.body, BODY_LOCAL);
    checkLocal(fixture.edit, kart.wheel, WHEEL_LOCAL);
    checkPlacedMesh(fixture.edit, kart.body, EDIT_MESH_ID);
    checkPlacedMesh(fixture.edit, kart.wheel, EDIT_SECOND_MESH_ID);
    CHECK(fixture.edit.snapshot().hierarchy.size() == 5);
    CHECK(fixture.editRevision() > revisionBefore);
}

TEST_CASE("a mesh placed on an entity becomes its first child", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeMeshUnderEntity"};
    const Entity parent = fixture.edit.createWithMeshRenderer("Parent");
    const Entity existingChild = fixture.edit.scene.createEntity();
    REQUIRE(fixture.edit.scene.setParent(existingChild, parent, 0));

    fixture.placeUnder(HARNESS_MESH_PATH, parent);

    const std::vector<Entity> children = fixture.edit.scene.getChildren(parent);
    REQUIRE(children.size() == 2);
    CHECK(children[1] == existingChild);
    CHECK(fixture.edit.scene.getComponent<Name>(children[0]).value == "Triangle");
    checkLocal(fixture.edit, children[0], TransformValues{});
    checkPlacedMesh(fixture.edit, children[0], EDIT_MESH_ID);
}

TEST_CASE("a model placed on an entity the scene no longer holds places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeUnderDeadEntity"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    const Entity parent = fixture.edit.createWithMeshRenderer("Parent");
    DestroyEntityCommand destroy{fixture.edit.session, {parent}};
    destroy.apply(fixture.edit.scene);
    REQUIRE_FALSE(fixture.edit.scene.contains(parent));
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.placeUnder(KART_MODEL_PATH, parent);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a one-node model whose root carries a mesh places one entity with its renderer", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeLoneRoot"};
    Model model;
    model.id = KART_MODEL_ID;
    model.nodes.push_back(makeNode("Kart", MODEL_NO_PARENT, BODY_LOCAL, EDIT_MESH_ID));
    fixture.addModel(model, KART_MODEL_PATH);

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK(fixture.edit.snapshot().hierarchy.size() == 1);
    CHECK(fixture.edit.scene.getComponent<Name>(roots[0]).value == "Kart");
    checkLocal(fixture.edit, roots[0], BODY_LOCAL);
    checkPlacedMesh(fixture.edit, roots[0], EDIT_MESH_ID);
}

TEST_CASE("placing a model fills each renderer's slot list from its mesh's slots and defaults", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeFillsSlots"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    const PlacedKart kart = walkKart(fixture.edit, roots[0]);
    const std::vector<MaterialSlot> expected{MaterialSlot{"Slot 0", {}}};
    CHECK(fixture.edit.scene.getComponent<MeshRenderer>(kart.body).slots == expected);
    CHECK(fixture.edit.scene.getComponent<MeshRenderer>(kart.wheel).slots == expected);
}

TEST_CASE("a model with a mesh that fails to load places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeBrokenMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_MESH_ID, EDIT_BROKEN_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with a mesh no file holds places that node with its mesh missing, and warns", "[editor][edit][commands][place][os]") {
    const TechEngineTests::LogCaptureGuard logs;
    PlaceFixture fixture{"placeMissingMesh"};
    ResourceId missingMesh;

    SECTION("a mesh the index does not know") {
        missingMesh = EDIT_UNKNOWN_ID;
    }

    SECTION("an indexed mesh whose bytes the source does not hold") {
        missingMesh = ResourceId{0x3E540000000000AA};
        AssetRecord record;
        record.id = missingMesh;
        record.typeTag = TechEngine::MESH_TYPE_TAG;
        record.side = TechEngine::Side::Client;
        record.path = "assets/client/Lost.temesh";
        fixture.edit.index.add(std::move(record));
    }

    fixture.addModel(makeKart(TransformValues{}, EDIT_MESH_ID, missingMesh), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    const PlacedKart kart = walkKart(fixture.edit, roots[0]);
    checkPlacedMesh(fixture.edit, kart.body, EDIT_MESH_ID);
    REQUIRE(fixture.edit.scene.hasComponent<MeshRenderer>(kart.wheel));
    const MeshRenderer& wheel = fixture.edit.scene.getComponent<MeshRenderer>(kart.wheel);
    CHECK(wheel.mesh.id == missingMesh);
    REQUIRE(wheel.mesh.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(fixture.edit.cache.getState(wheel.mesh.handle, state));
    CHECK(state == ResourceState::Missing);
    CHECK(fixture.edit.cache.resolve(wheel.mesh.handle) == nullptr);
    CHECK(wheel.slots.empty());
    CHECK(fixture.editRevision() > revisionBefore);
    const std::string idText = std::format("{:#018x}", missingMesh.value());
    CHECK(std::ranges::any_of(logs.getMessages(), [&idText](const std::string& message) {
        return message.find(idText) != std::string::npos && message.find("no file holds") != std::string::npos;
    }));
}

TEST_CASE("a model with a server mesh places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeServerMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_SERVER_MESH_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a path the index does not know places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeUnknownPath"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(MISSING_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a path that names neither a model nor a mesh places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeScenePath"};
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(HARNESS_SCENE_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("placing a merged mesh adds one entity whose slot list matches the mesh", "[editor][edit][commands][place][merged][os]") {
    PlaceFixture fixture{"placeMergedMesh"};
    TechEngine::Mesh mesh;
    mesh.id = MERGED_MESH_ID;
    mesh.positions = {Vec3(0.0f), Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f)};
    mesh.indices = {0, 1, 2, 0, 3, 1};
    mesh.slots = {TechEngine::MeshSlot{"Paint", 0, 3, EDIT_MATERIAL_ID}, TechEngine::MeshSlot{"Glass", 3, 3, ResourceId{}}};
    std::vector<std::byte> bytes;
    mesh.write(bytes);
    fixture.edit.source.addAsset(MERGED_MESH_ID, std::move(bytes));
    AssetRecord record;
    record.id = MERGED_MESH_ID;
    record.typeTag = TechEngine::MESH_TYPE_TAG;
    record.side = TechEngine::Side::Client;
    record.path = std::string(MERGED_MESH_PATH);
    fixture.edit.index.add(std::move(record));
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(MERGED_MESH_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    const Entity placed = roots[0];
    CHECK(fixture.edit.snapshot().hierarchy.size() == 1);
    CHECK(fixture.edit.scene.getChildren(placed).empty());
    CHECK(fixture.edit.scene.getComponent<Name>(placed).value == "Front");
    checkLocal(fixture.edit, placed, TransformValues{});
    checkPlacedMesh(fixture.edit, placed, MERGED_MESH_ID);
    CHECK(fixture.editRevision() > revisionBefore);

    const std::vector<MaterialSlot>& slots = fixture.edit.scene.getComponent<MeshRenderer>(placed).slots;
    REQUIRE(slots.size() == 2);
    CHECK(slots[0].name == "Paint");
    CHECK(slots[0].material.id == EDIT_MATERIAL_ID);
    REQUIRE(slots[0].material.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(fixture.edit.cache.getState(slots[0].material.handle, state));
    CHECK(state == ResourceState::Ready);
    CHECK(slots[1] == MaterialSlot{"Glass", {}});
}

TEST_CASE("placing an imported split mesh names its children after their materials, with _1 on a repeat", "[editor][edit][commands][place][import][os]") {
    PlaceFixture fixture{"placeSplitMaterials"};
    fixture.scratch.writeFile("assets/client/Car.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_SPLIT_MATERIALS_BODY));
    TechEngine::ModelImportRequest request;
    request.sourcePath = "project://assets/client/Car.gltf";
    request.side = TechEngine::Side::Client;
    std::uint64_t nextId = SPLIT_FIRST_ID;
    const TechEngine::ModelImporter importer(fixture.files, [&nextId] {
        return ResourceId{nextId++};
    });
    TechEngine::ModelImportResult result;
    REQUIRE(importer.importFile(request, result) == std::error_code{});
    for (const AssetRecord& record: result.records) {
        std::vector<std::byte> bytes;
        REQUIRE(fixture.files.read(record.path, bytes) == std::error_code{});
        fixture.edit.source.addAsset(record.id, std::move(bytes));
        fixture.edit.index.add(record);
    }

    fixture.place(result.records[0].path);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK(fixture.edit.scene.getComponent<Name>(roots[0]).value == "Car");
    const std::vector<Entity> children = fixture.edit.scene.getChildren(roots[0]);
    REQUIRE(children.size() == 3);
    CHECK(fixture.edit.scene.getComponent<Name>(children[0]).value == "Paint");
    CHECK(fixture.edit.scene.getComponent<Name>(children[1]).value == "Glass");
    CHECK(fixture.edit.scene.getComponent<Name>(children[2]).value == "Paint_1");
    for (const Entity child: children) {
        CHECK(fixture.edit.scene.hasComponent<MeshRenderer>(child));
    }
}

TEST_CASE("a server mesh is not placed", "[editor][edit][commands][place][merged][os]") {
    PlaceFixture fixture{"placeServerMergedMesh"};
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(HARNESS_SERVER_MESH_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("an indexed model whose file is gone or does not parse places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeUnreadableModel"};
    const std::vector<std::byte> garbage = {std::byte{0x6E}, std::byte{0x6F}, std::byte{0x70}, std::byte{0x65}};
    fixture.addModelBytes(ResourceId{0x6D6F64656C000102}, garbage, BROKEN_MODEL_PATH);
    fixture.indexModel(ResourceId{0x6D6F64656C000103}, MISSING_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(BROKEN_MODEL_PATH);
    fixture.place(MISSING_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("placing on a scene with no cache entry places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeNoSceneEntry"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    fixture.edit.releaseSceneEntry();
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model that names a scene as a node's mesh places nothing", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeNotAMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_OTHER_SCENE_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with a zero scale on its last node places nothing and removes the entities built before it", "[editor][edit][commands][place][os]") {
    PlaceFixture fixture{"placeZeroScale"};
    Model model = makeKart();
    model.nodes[2].local.scale = Vec3(1.0f, 0.0f, 1.0f);
    fixture.addModel(model, KART_MODEL_PATH);
    const Entity existing = fixture.edit.createWithMeshRenderer("Existing");
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    const std::vector<Entity> roots = fixture.edit.scene.getRoots();
    REQUIRE(roots.size() == 1);
    CHECK(roots[0] == existing);
    CHECK(fixture.edit.snapshot().hierarchy.size() == 1);
    CHECK(fixture.editRevision() == revisionBefore);
}
