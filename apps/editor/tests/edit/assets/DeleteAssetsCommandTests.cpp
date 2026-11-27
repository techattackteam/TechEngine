#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/testing/LogCapture.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/assets/DeleteAssetsCommand.hpp>
#include <edit/assets/DeleteTestProject.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::AssetRef;
using TechEngine::DeleteAssetsCommand;
using TechEngine::ListedEntry;
using TechEngine::Material;
using TechEngine::Mesh;
using TechEngine::ProjectError;
using TechEngineTests::DeleteTestProject;
using TechEngineTests::projectPath;

static void deletePaths(DeleteTestProject& project, const std::vector<std::string_view>& relatives) {
    std::vector<std::string> paths;
    for (const std::string_view relative: relatives) {
        paths.push_back(projectPath(relative));
    }
    DeleteAssetsCommand command{project.session, project.files, project.index, project.jobs, project.createBatches, std::move(paths)};
    command.apply(project.scene);
}

static bool isListed(const DeleteTestProject& project, const std::string_view relative) {
    const std::string path = projectPath(relative);
    return std::ranges::find(project.index.getListing(), path, &ListedEntry::path) != project.index.getListing().end();
}

static bool warned(const TechEngineTests::LogCaptureGuard& logs) {
    const std::vector<TechEngine::StoredLogRecord> records = logs.getRecords();
    return std::ranges::any_of(records, [](const TechEngine::StoredLogRecord& logged) {
        return logged.level == TechEngine::Level::Warn;
    });
}

TEST_CASE("a delete removes the selected files and their index records, and lists the assets again", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteRemovesFiles"};
    const std::uint64_t revision = project.index.getListingRevision();

    deletePaths(project, {TechEngineTests::DELETE_KEPT_MESH, TechEngineTests::DELETE_BRICK});

    CHECK_FALSE(project.exists(TechEngineTests::DELETE_KEPT_MESH));
    CHECK_FALSE(project.exists(TechEngineTests::DELETE_BRICK));
    CHECK_FALSE(project.isIndexed(TechEngineTests::DELETE_KEPT_MESH_ID));
    CHECK_FALSE(project.isIndexed(TechEngineTests::DELETE_BRICK_ID));
    CHECK_FALSE(isListed(project, TechEngineTests::DELETE_KEPT_MESH));
    CHECK(project.index.getListingRevision() == revision + 1);

    CHECK(project.exists(TechEngineTests::DELETE_MESH));
    CHECK(project.isIndexed(TechEngineTests::DELETE_MESH_ID));
    CHECK(project.isIndexed(TechEngineTests::DELETE_PAINT_ID));
}

TEST_CASE("a deleted folder takes everything in it, and a folder beside it that shares its name's start stays", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteRemovesFolder"};

    deletePaths(project, {TechEngineTests::DELETE_CRATE_FOLDER});

    CHECK_FALSE(project.exists(TechEngineTests::DELETE_CRATE_FOLDER));
    CHECK_FALSE(project.isIndexed(TechEngineTests::DELETE_CRATE_MESH_ID));
    CHECK_FALSE(project.isIndexed(TechEngineTests::DELETE_CRATE_MATERIAL_ID));
    CHECK_FALSE(isListed(project, TechEngineTests::DELETE_CRATE_FOLDER));
    CHECK(project.exists(TechEngineTests::DELETE_CRATE_MODEL));
    CHECK(project.exists(TechEngineTests::DELETE_CRATE_SOURCE));
    CHECK(project.isIndexed(TechEngineTests::DELETE_CRATE_MODEL_ID));
}

TEST_CASE("deleting a source deletes that file only, and the folder created from it stays", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteSourceOnly"};

    deletePaths(project, {TechEngineTests::DELETE_CRATE_SOURCE});

    CHECK_FALSE(project.exists(TechEngineTests::DELETE_CRATE_SOURCE));
    CHECK_FALSE(isListed(project, TechEngineTests::DELETE_CRATE_SOURCE));
    CHECK(project.exists(TechEngineTests::DELETE_CRATE_MESH));
    CHECK(project.isIndexed(TechEngineTests::DELETE_CRATE_MESH_ID));
    CHECK(project.isIndexed(TechEngineTests::DELETE_CRATE_MODEL_ID));
}

TEST_CASE("the open scene's references to a deleted mesh are emptied, its slots stay, and the scene is unsaved", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteEmptiesMesh"};
    REQUIRE_FALSE(project.session.hasUnsavedEdits());

    deletePaths(project, {TechEngineTests::DELETE_MESH});

    const TechEngine::MeshRenderer& painted = project.rendererOf(project.painted);
    CHECK(painted.mesh == AssetRef<Mesh>{});
    REQUIRE(painted.slots.size() == 1);
    CHECK(painted.slots[0].name == "Slot 0");
    CHECK(painted.slots[0].material.id == TechEngineTests::DELETE_PAINT_ID);
    CHECK(project.rendererOf(project.kept).mesh.id == TechEngineTests::DELETE_KEPT_MESH_ID);
    CHECK(project.session.hasUnsavedEdits());
}

TEST_CASE("the open scene's slot that names a deleted material is emptied, and its mesh stays", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteEmptiesMaterial"};

    deletePaths(project, {TechEngineTests::DELETE_PAINT});

    const TechEngine::MeshRenderer& painted = project.rendererOf(project.painted);
    CHECK(painted.mesh.id == TechEngineTests::DELETE_MESH_ID);
    REQUIRE(painted.slots.size() == 1);
    CHECK(painted.slots[0].name == "Slot 0");
    CHECK(painted.slots[0].material == AssetRef<Material>{});
    CHECK(project.session.hasUnsavedEdits());
}

TEST_CASE("a deleted folder empties the open scene's references to everything it held", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteFolderEmptiesReferences"};
    TechEngine::MeshRenderer renderer;
    renderer.mesh.id = TechEngineTests::DELETE_CRATE_MESH_ID;
    renderer.slots.push_back(TechEngine::MaterialSlot{"Slot 0", AssetRef<Material>{TechEngineTests::DELETE_CRATE_MATERIAL_ID, {}}});
    const TechEngine::Entity crate = project.scene.createEntity();
    REQUIRE(crate.valid());
    project.scene.addComponent<TechEngine::MeshRenderer>(crate, std::move(renderer));

    deletePaths(project, {TechEngineTests::DELETE_CRATE_FOLDER});

    const TechEngine::MeshRenderer& cleared = project.rendererOf(crate);
    CHECK(cleared.mesh == AssetRef<Mesh>{});
    REQUIRE(cleared.slots.size() == 1);
    CHECK(cleared.slots[0].material == AssetRef<Material>{});
    CHECK(project.rendererOf(project.painted).mesh.id == TechEngineTests::DELETE_MESH_ID);
}

TEST_CASE("the open scene's own file is refused, and nothing is deleted", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteRefusesOpenScene"};
    const std::uint64_t revision = project.index.getListingRevision();
    const TechEngineTests::LogCaptureGuard logs;

    SECTION("the file itself, beside a file that could be deleted") {
        deletePaths(project, {TechEngineTests::DELETE_OPEN_SCENE, TechEngineTests::DELETE_KEPT_MESH});

        CHECK(project.exists(TechEngineTests::DELETE_OPEN_SCENE));
        CHECK(project.exists(TechEngineTests::DELETE_KEPT_MESH));
    }

    SECTION("a folder that holds it") {
        deletePaths(project, {TechEngineTests::DELETE_SCENES_FOLDER});

        CHECK(project.exists(TechEngineTests::DELETE_OPEN_SCENE));
    }

    CHECK(project.isIndexed(TechEngineTests::DELETE_OPEN_SCENE_ID));
    CHECK(project.isIndexed(TechEngineTests::DELETE_KEPT_MESH_ID));
    CHECK(project.index.getListingRevision() == revision);
    CHECK(project.rendererOf(project.kept).mesh.id == TechEngineTests::DELETE_KEPT_MESH_ID);
    CHECK_FALSE(project.session.hasUnsavedEdits());
    CHECK(warned(logs));
}

TEST_CASE("a delete while a create runs is refused, and the same delete applies once the create finishes", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteRefusesDuringCreate"};
    project.startCreate();
    const TechEngineTests::LogCaptureGuard logs;

    deletePaths(project, {TechEngineTests::DELETE_KEPT_MESH});

    CHECK(project.exists(TechEngineTests::DELETE_KEPT_MESH));
    CHECK(project.isIndexed(TechEngineTests::DELETE_KEPT_MESH_ID));
    CHECK(warned(logs));

    project.releaseCreate();
    project.jobs.wait(project.createBatches.back());

    deletePaths(project, {TechEngineTests::DELETE_KEPT_MESH});

    CHECK_FALSE(project.exists(TechEngineTests::DELETE_KEPT_MESH));
    CHECK_FALSE(project.isIndexed(TechEngineTests::DELETE_KEPT_MESH_ID));
}

TEST_CASE("findRefusal names the reason", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteFindRefusal"};
    const std::vector<std::string> openScene = {projectPath(TechEngineTests::DELETE_OPEN_SCENE)};
    const std::vector<std::string> kept = {projectPath(TechEngineTests::DELETE_KEPT_MESH)};
    const std::vector<std::string> lookalike = {projectPath("assets/common/Scenes/Open.tescene-old")};

    CHECK(DeleteAssetsCommand::findRefusal(project.session, project.jobs, project.createBatches, openScene) == std::error_code{ProjectError::DeletesOpenScene});
    CHECK(DeleteAssetsCommand::findRefusal(project.session, project.jobs, project.createBatches, kept) == std::error_code{});
    CHECK(DeleteAssetsCommand::findRefusal(project.session, project.jobs, project.createBatches, lookalike) == std::error_code{});

    project.startCreate();
    CHECK(DeleteAssetsCommand::findRefusal(project.session, project.jobs, project.createBatches, kept) == std::error_code{ProjectError::CreateInProgress});
}

TEST_CASE("collectDeletedRecords takes the records at and under each path, and not a sibling that shares its name's start", "[editor][edit][delete][os]") {
    DeleteTestProject project{"deleteCollectRecords"};
    const std::vector<std::string> paths = {projectPath(TechEngineTests::DELETE_CRATE_FOLDER), projectPath(TechEngineTests::DELETE_BRICK)};
    std::vector<AssetRecord> records;

    DeleteAssetsCommand::collectDeletedRecords(project.index, paths, records);

    std::vector<TechEngine::ResourceId> ids;
    for (const AssetRecord& record: records) {
        ids.push_back(record.id);
    }
    std::ranges::sort(ids);
    std::vector<TechEngine::ResourceId> expected = {TechEngineTests::DELETE_BRICK_ID, TechEngineTests::DELETE_CRATE_MESH_ID, TechEngineTests::DELETE_CRATE_MATERIAL_ID};
    std::ranges::sort(expected);
    CHECK(ids == expected);
}
