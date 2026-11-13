#include <TechEngine/app/App.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <assets/AssetIndex.hpp>
#include <assets/ModelImporter.hpp>
#include <assets/ModelTestFiles.hpp>
#include <edit/AssetEditHarness.hpp>
#include <edit/EditSession.hpp>
#include <edit/ReimportCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::AssetRef;
using TechEngine::Entity;
using TechEngine::Material;
using TechEngine::Mesh;
using TechEngine::MeshRenderer;
using TechEngine::PlayState;
using TechEngine::ReimportCommand;
using TechEngine::ReloadImportedAssetsCommand;
using TechEngine::ResourceHandle;
using TechEngine::ResourceId;
using TechEngine::ResourceSlotId;
using TechEngine::ResourceState;
using TechEngineTests::AppTestAccess;
using TechEngineTests::AssetEditHarness;
using TechEngineTests::EDIT_MESH_ID;
using TechEngineTests::ScratchDirectory;

static constexpr std::string_view PAIR_SOURCE = "project://assets/client/Pair.gltf";
static constexpr std::string_view PAIR_MODEL = "project://assets/client/Pair/Pair.temodel";
static constexpr ResourceId NEW_MATERIAL_ID{0x3A7E000000000002};

class ReimportAppProbe final : public TechEngine::App {
public:
    ScratchDirectory scratch;
    TechEngine::AssetIndex index;
    TechEngine::EditSession session;
    std::vector<TechEngine::BatchId> batches;

    explicit ReimportAppProbe(const std::string_view name) : App(TechEngine::Role::DedicatedServer), scratch(name) {
        m_mounts.mount("project", scratch.root());
        scratch.makeDirectory("assets/client");
    }

    const TechEngine::FileAccess& files() const {
        return m_files;
    }

    void importPair() {
        scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));
        TechEngine::ModelImportRequest request;
        request.sourcePath = std::string(PAIR_SOURCE);
        request.side = TechEngine::Side::Client;
        TechEngine::ModelImportResult result;
        const TechEngine::ModelImporter importer(m_files, &TechEngine::randomResourceId);
        REQUIRE(importer.importFile(request, result) == std::error_code{});
    }

    ReimportCommand makeReimport(const std::string_view sourcePath) {
        TechEngine::ReimportRequest request;
        request.sourcePath = std::string(sourcePath);
        return ReimportCommand{*this, m_jobs, m_files, index, session, m_resources, m_sceneHandle, batches, std::move(request)};
    }

    void waitForImports() {
        for (const TechEngine::BatchId batch: batches) {
            m_jobs.wait(batch);
        }
    }

    std::vector<std::byte> bytesOf(const std::string_view path) const {
        std::vector<std::byte> bytes;
        REQUIRE(m_files.read(path, bytes) == std::error_code{});
        return bytes;
    }

protected:
    void init() override {
    }
};

static std::vector<std::byte> twoSlotMeshBytes() {
    Mesh mesh;
    mesh.id = EDIT_MESH_ID;
    mesh.positions = {TechEngine::Vec3(-0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.5f, -0.5f, 0.0f), TechEngine::Vec3(0.0f, 0.5f, 0.0f)};
    mesh.indices = {0, 1, 2, 2, 1, 0};
    mesh.slots = {TechEngine::MeshSlot{"Slot 0", 0, 3, ResourceId{}}, TechEngine::MeshSlot{"Trim", 3, 3, NEW_MATERIAL_ID}};
    std::vector<std::byte> bytes;
    mesh.write(bytes);
    return bytes;
}

static std::vector<std::byte> newMaterialBytes() {
    Material material;
    material.id = NEW_MATERIAL_ID;
    std::vector<std::byte> bytes;
    material.write(bytes);
    return bytes;
}

static AssetRecord newMaterialRecord() {
    AssetRecord record;
    record.id = NEW_MATERIAL_ID;
    record.typeTag = TechEngine::MATERIAL_TYPE_TAG;
    record.side = TechEngine::Side::Client;
    record.path = "assets/client/Trim.tematerial";
    return record;
}

static Entity createHoldingMesh(AssetEditHarness& edit, const std::string_view name, const ResourceId mesh) {
    const Entity entity = edit.createWithMeshRenderer(name);
    ResourceSlotId slot;
    REQUIRE(edit.cache.addDependency(edit.sceneHandle, mesh, slot) == std::error_code{});
    REQUIRE(edit.cache.runLoadingPhase() == std::error_code{});
    MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    renderer.mesh = AssetRef<Mesh>{mesh, ResourceHandle<Mesh>{slot.index, slot.generation}};
    const Mesh* loaded = edit.cache.resolve(renderer.mesh.handle);
    REQUIRE(loaded != nullptr);
    std::vector<std::string> dropped;
    renderer.rebuildSlots(*loaded, dropped);
    return entity;
}

TEST_CASE("a re-import is refused while the editor is not Stopped, and starts no job", "[editor][edit][reimport]") {
    ReimportAppProbe app{"reimportRefusedPlaying"};
    app.importPair();
    const std::vector<std::byte> before = app.bytesOf(PAIR_MODEL);

    SECTION("Playing") {
        app.session.setPlayState(PlayState::Playing);
    }

    SECTION("Paused") {
        app.session.setPlayState(PlayState::Paused);
    }

    app.makeReimport(PAIR_SOURCE).apply(AppTestAccess::scene(app));
    app.waitForImports();

    CHECK(app.batches.empty());
    CHECK(app.bytesOf(PAIR_MODEL) == before);
}

TEST_CASE("a Stopped re-import runs as a job and rewrites the files under their ids", "[editor][edit][reimport]") {
    ReimportAppProbe app{"reimportStoppedJob"};
    app.importPair();
    TechEngine::Model before;
    REQUIRE(before.read(app.bytesOf(PAIR_MODEL)) == std::error_code{});
    REQUIRE(app.session.getPlayState() == PlayState::Stopped);

    app.makeReimport(PAIR_SOURCE).apply(AppTestAccess::scene(app));

    REQUIRE(app.batches.size() == 1);
    app.waitForImports();
    TechEngine::Model after;
    REQUIRE(after.read(app.bytesOf(PAIR_MODEL)) == std::error_code{});
    CHECK(after.id == before.id);
    REQUIRE(after.nodes.size() == before.nodes.size());
    for (std::size_t i = 0; i < before.nodes.size(); i++) {
        CHECK(after.nodes[i].mesh == before.nodes[i].mesh);
    }

    SECTION("with no simulation running, the reload is refused and the index stays as it was") {
        CHECK(app.index.getRecords().empty());
    }
}

TEST_CASE("a re-import of a source with no folder is refused by its job and writes nothing", "[editor][edit][reimport]") {
    ReimportAppProbe app{"reimportNoFolder"};
    app.scratch.writeFile("assets/client/Pair.gltf", TechEngineTests::gltfFile(TechEngineTests::GLTF_PAIR_BODY));

    app.makeReimport(PAIR_SOURCE).apply(AppTestAccess::scene(app));
    app.waitForImports();

    CHECK_FALSE(std::filesystem::exists(app.scratch.root() / "assets" / "client" / "Pair"));
}

TEST_CASE("the reload command replaces a mesh behind its live handle and rebuilds the open scene's slots", "[editor][edit][reimport]") {
    AssetEditHarness edit;
    const Entity entity = createHoldingMesh(edit, "Crate", EDIT_MESH_ID);
    const ResourceHandle<Mesh> handle = edit.scene.getComponent<MeshRenderer>(entity).mesh.handle;
    REQUIRE(edit.cache.resolve(handle)->slots.size() == 1);
    REQUIRE(edit.scene.getComponent<MeshRenderer>(entity).slots.size() == 1);
    const ResourceHandle<TechEngine::SceneBytes> sceneHandle = edit.sceneHandle;
    edit.source.add(EDIT_MESH_ID, twoSlotMeshBytes());
    edit.source.add(NEW_MATERIAL_ID, newMaterialBytes());

    ReloadImportedAssetsCommand command{edit.session, edit.cache, edit.sceneHandle, edit.index, {newMaterialRecord()}, {EDIT_MESH_ID}};
    command.apply(edit.scene);

    const MeshRenderer& renderer = edit.scene.getComponent<MeshRenderer>(entity);
    CHECK(renderer.mesh.handle == handle);
    const Mesh* mesh = edit.cache.resolve(handle);
    REQUIRE(mesh != nullptr);
    REQUIRE(mesh->slots.size() == 2);
    CHECK(mesh->slots[1].name == "Trim");

    REQUIRE(renderer.slots.size() == 2);
    CHECK(renderer.slots[0].name == "Slot 0");
    CHECK_FALSE(renderer.slots[0].material.id.valid());
    CHECK(renderer.slots[1].name == "Trim");
    CHECK(renderer.slots[1].material.id == NEW_MATERIAL_ID);
    REQUIRE(renderer.slots[1].material.handle.valid());
    ResourceState state = ResourceState::Pending;
    REQUIRE(edit.cache.getState(renderer.slots[1].material.handle, state));
    CHECK(state == ResourceState::Ready);

    AssetRecord found;
    REQUIRE(edit.index.find(NEW_MATERIAL_ID, found));
    CHECK(found.path == "assets/client/Trim.tematerial");
    REQUIRE(edit.index.find(EDIT_MESH_ID, found));
    CHECK(found.path == "assets/client/Triangle.temesh");

    CHECK(edit.sceneHandle == sceneHandle);
    REQUIRE(edit.cache.getState(edit.sceneHandle, state));
    CHECK(state == ResourceState::Ready);
}

TEST_CASE("the reload command reloads nothing while the editor is not Stopped, but still indexes the new files", "[editor][edit][reimport]") {
    AssetEditHarness edit;
    const Entity entity = createHoldingMesh(edit, "Crate", EDIT_MESH_ID);
    const ResourceHandle<Mesh> handle = edit.scene.getComponent<MeshRenderer>(entity).mesh.handle;
    edit.source.add(EDIT_MESH_ID, twoSlotMeshBytes());
    edit.session.setPlayState(PlayState::Playing);

    ReloadImportedAssetsCommand command{edit.session, edit.cache, edit.sceneHandle, edit.index, {newMaterialRecord()}, {EDIT_MESH_ID}};
    command.apply(edit.scene);

    REQUIRE(edit.cache.resolve(handle) != nullptr);
    CHECK(edit.cache.resolve(handle)->slots.size() == 1);
    CHECK(edit.scene.getComponent<MeshRenderer>(entity).slots.size() == 1);
    AssetRecord found;
    CHECK(edit.index.find(NEW_MATERIAL_ID, found));
}
