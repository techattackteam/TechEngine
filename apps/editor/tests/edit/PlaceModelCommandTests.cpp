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
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/AssetEditHarness.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/HierarchyCommands.hpp>
#include <edit/PlaceModelCommand.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
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
// The harness indexes its meshes under paths with no mount.
static constexpr std::string_view HARNESS_MESH_PATH = "assets/client/Triangle.temesh";

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
        PlaceModelCommand command{edit.session, files, edit.cache, edit.sceneHandle, edit.index, std::string(path)};
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

TEST_CASE("placing a three-node model adds three entities with their parents and local transforms", "[editor][edit][commands][place]") {
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

TEST_CASE("placing a model goes beside the scene's own entities and leaves them as they were", "[editor][edit][commands][place]") {
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

TEST_CASE("placing a model twice gives two independent trees", "[editor][edit][commands][place]") {
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

TEST_CASE("editing one placed copy changes neither the other copy nor the model file", "[editor][edit][commands][place]") {
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

TEST_CASE("a placed root keeps the transform the model stores for it", "[editor][edit][commands][place]") {
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

TEST_CASE("a one-node model whose root carries a mesh places one entity with its renderer", "[editor][edit][commands][place]") {
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

TEST_CASE("a model with a mesh that fails to load places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeBrokenMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_MESH_ID, EDIT_BROKEN_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with a mesh the index does not know places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeUnknownMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_MESH_ID, EDIT_UNKNOWN_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with a server mesh places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeServerMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_SERVER_MESH_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a path the index does not know places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeUnknownPath"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(MISSING_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a path that names a mesh rather than a model places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeMeshPath"};
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(HARNESS_MESH_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("an indexed model whose file is gone or does not parse places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeUnreadableModel"};
    const std::vector<std::byte> garbage = {std::byte{0x6E}, std::byte{0x6F}, std::byte{0x70}, std::byte{0x65}};
    fixture.addModelBytes(ResourceId{0x6D6F64656C000102}, garbage, BROKEN_MODEL_PATH);
    fixture.indexModel(ResourceId{0x6D6F64656C000103}, MISSING_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(BROKEN_MODEL_PATH);
    fixture.place(MISSING_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("placing on a scene with no cache entry places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeNoSceneEntry"};
    fixture.addModel(makeKart(), KART_MODEL_PATH);
    fixture.edit.releaseSceneEntry();
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model that names a scene as a node's mesh places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeNotAMesh"};
    fixture.addModel(makeKart(TransformValues{}, EDIT_OTHER_SCENE_ID, EDIT_SECOND_MESH_ID), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with an indexed mesh whose bytes the cache cannot find places nothing", "[editor][edit][commands][place]") {
    PlaceFixture fixture{"placeLostMesh"};
    const ResourceId lostMesh{0x3E540000000000AA};
    AssetRecord record;
    record.id = lostMesh;
    record.typeTag = TechEngine::MESH_TYPE_TAG;
    record.side = TechEngine::Side::Client;
    record.path = "assets/client/Lost.temesh";
    fixture.edit.index.add(std::move(record));
    fixture.addModel(makeKart(TransformValues{}, EDIT_MESH_ID, lostMesh), KART_MODEL_PATH);
    const std::uint64_t revisionBefore = fixture.editRevision();

    fixture.place(KART_MODEL_PATH);

    checkNothingPlaced(fixture, revisionBefore);
}

TEST_CASE("a model with a zero scale on its last node places nothing and removes the entities built before it", "[editor][edit][commands][place]") {
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
