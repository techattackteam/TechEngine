#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/assets/DeletePreview.hpp>
#include <edit/assets/DeleteTestProject.hpp>
#include <edit/assets/PreviewDeleteCommand.hpp>
#include <edit/snapshot/EditorSnapshot.hpp>
#include <project/ProjectError.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

using TechEngine::AssetRecord;
using TechEngine::DeletePreview;
using TechEngine::EditorSnapshot;
using TechEngine::PreviewDeleteCommand;
using TechEngine::ProjectError;
using TechEngineTests::DeleteTestProject;
using TechEngineTests::projectPath;

static DeletePreview previewPaths(DeleteTestProject& project, const std::vector<std::string_view>& relatives, const std::uint64_t sequence = 1) {
    std::vector<std::string> paths;
    for (const std::string_view relative: relatives) {
        paths.push_back(projectPath(relative));
    }
    PreviewDeleteCommand command{project.session, project.getRegistry(), project.files, project.cache, project.index, project.jobs, project.createBatches, std::move(paths), sequence};
    command.apply(project.scene);

    EditorSnapshot snapshot;
    project.session.buildSnapshot(project.scene, project.getRegistry(), snapshot);
    return snapshot.deletePreview;
}

static std::vector<std::string> sorted(std::vector<std::string> paths) {
    std::ranges::sort(paths);
    return paths;
}

static std::vector<std::string> projectPaths(const std::vector<std::string_view>& relatives) {
    std::vector<std::string> paths;
    for (const std::string_view relative: relatives) {
        paths.push_back(projectPath(relative));
    }
    return paths;
}

static bool isScene(const DeleteTestProject& project, const std::string& path) {
    const auto found = std::ranges::find(project.index.getRecords(), path, &AssetRecord::path);
    return found != project.index.getRecords().end() && found->typeTag == TechEngine::SCENE_TYPE_TAG;
}

TEST_CASE("the preview answers the request it was sent, through the snapshot", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewAnswers"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_KEPT_MESH}, 7);

    CHECK(preview.sequence == 7);
    CHECK(preview.paths == projectPaths({TechEngineTests::DELETE_KEPT_MESH}));
    CHECK(preview.refusal == std::error_code{});

    SECTION("a later request replaces it") {
        const DeletePreview later = previewPaths(project, {TechEngineTests::DELETE_BRICK}, 8);

        CHECK(later.sequence == 8);
        CHECK(later.paths == projectPaths({TechEngineTests::DELETE_BRICK}));
    }
}

TEST_CASE("the preview lists every scene that names a deleted mesh, scenes first", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewSceneUsers"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_MESH});

    CHECK(sorted(preview.users) == sorted(projectPaths({TechEngineTests::DELETE_OPEN_SCENE, TechEngineTests::DELETE_OTHER_SCENE})));
    CHECK(preview.heldFileCount == 0);
}

TEST_CASE("the preview lists a mesh whose slot defaults to a deleted material, after the scenes", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewMeshUser"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_PAINT});

    REQUIRE(sorted(preview.users) == sorted(projectPaths({TechEngineTests::DELETE_OPEN_SCENE, TechEngineTests::DELETE_MESH})));
    CHECK(isScene(project, preview.users.front()));
    CHECK_FALSE(isScene(project, preview.users.back()));
}

TEST_CASE("the preview lists a material that names a deleted texture", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewMaterialUser"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_BRICK});

    CHECK(preview.users == projectPaths({TechEngineTests::DELETE_PAINT}));
}

TEST_CASE("the preview lists a model that names a deleted mesh", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewModelUser"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_CRATE_MESH});

    CHECK(preview.users == projectPaths({TechEngineTests::DELETE_CRATE_MODEL}));
}

TEST_CASE("the preview lists the open scene when only its unsaved edits name the selection", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewUnsavedUser"};
    TechEngine::MeshRenderer renderer;
    renderer.mesh.id = TechEngineTests::DELETE_CRATE_MESH_ID;
    const TechEngine::Entity placed = project.scene.createEntity();
    REQUIRE(placed.valid());
    project.scene.addComponent<TechEngine::MeshRenderer>(placed, std::move(renderer));

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_CRATE_MESH});

    REQUIRE(sorted(preview.users) == sorted(projectPaths({TechEngineTests::DELETE_OPEN_SCENE, TechEngineTests::DELETE_CRATE_MODEL})));
    CHECK(preview.users.front() == projectPath(TechEngineTests::DELETE_OPEN_SCENE));
}

TEST_CASE("the preview of a folder counts its files and leaves out a user the delete removes too", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewFolder"};

    const DeletePreview preview = previewPaths(project, {TechEngineTests::DELETE_CRATE_FOLDER});

    CHECK(preview.users == projectPaths({TechEngineTests::DELETE_CRATE_MODEL}));
    CHECK(preview.heldFileCount == 2);
}

TEST_CASE("a file nothing names has no users", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewNoUsers"};

    SECTION("an asset") {
        CHECK(previewPaths(project, {TechEngineTests::DELETE_OTHER_SCENE}).users.empty());
    }

    SECTION("a source") {
        CHECK(previewPaths(project, {TechEngineTests::DELETE_CRATE_SOURCE}).users.empty());
    }
}

TEST_CASE("the preview carries the refusal the delete would give", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewRefusal"};

    SECTION("the open scene's folder") {
        CHECK(previewPaths(project, {TechEngineTests::DELETE_SCENES_FOLDER}).refusal == std::error_code{ProjectError::DeletesOpenScene});
    }

    SECTION("a create that is still running") {
        project.startCreate();

        CHECK(previewPaths(project, {TechEngineTests::DELETE_KEPT_MESH}).refusal == std::error_code{ProjectError::CreateInProgress});
    }
}

TEST_CASE("a preview deletes nothing and leaves the index and the scene as they were", "[editor][edit][delete][preview][os]") {
    DeleteTestProject project{"previewChangesNothing"};
    const std::uint64_t revision = project.index.getListingRevision();
    const std::size_t recordCount = project.index.getRecords().size();

    previewPaths(project, {TechEngineTests::DELETE_CRATE_FOLDER, TechEngineTests::DELETE_MESH});

    CHECK(project.exists(TechEngineTests::DELETE_CRATE_MESH));
    CHECK(project.exists(TechEngineTests::DELETE_MESH));
    CHECK(project.index.getListingRevision() == revision);
    CHECK(project.index.getRecords().size() == recordCount);
    CHECK(project.rendererOf(project.painted).mesh.id == TechEngineTests::DELETE_MESH_ID);
    CHECK_FALSE(project.session.hasUnsavedEdits());
}
