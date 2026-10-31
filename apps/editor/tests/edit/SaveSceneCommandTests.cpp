#include <TechEngine/app/App.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditTestScene.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/SaveSceneCommand.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

using TechEngine::AfterSave;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::ResourceId;
using TechEngine::SaveSceneCommand;
using TechEngine::SceneAsset;
using TechEngineTests::EditTestScene;
using TechEngineTests::InspectedAsset;
using TechEngineTests::InspectedSample;
using TechEngineTests::ScratchDirectory;

static constexpr ResourceId SAVED_SCENE_ID{0x5a5a5a5a00000001};
static constexpr std::string_view SAVED_SCENE_PATH = "project://Saved.tescene";
static constexpr std::string_view UNMOUNTED_SCENE_PATH = "missing://Saved.tescene";

class SaveAppProbe final : public TechEngine::App {
public:
    SaveAppProbe() : App(TechEngine::Role::DedicatedServer) {
    }

    bool isStopRequested() const {
        return stopRequested();
    }

protected:
    void init() override {
    }
};

class SaveFixture {
public:
    ScratchDirectory scratch;
    TechEngine::MountTable mounts;
    TechEngine::FileAccess files{mounts};
    EditTestScene edit;
    SaveAppProbe app;

    explicit SaveFixture(const std::string_view name) : scratch(name) {
        mounts.mount("project", scratch.root());
    }

    void save(const AfterSave after = AfterSave::KeepOpen) {
        SaveSceneCommand command{edit.session, files, app, after};
        command.apply(edit.scene);
    }

    std::ptrdiff_t fileCount() const {
        return std::distance(std::filesystem::directory_iterator{scratch.root()}, std::filesystem::directory_iterator{});
    }
};

TEST_CASE("a save and a reload give the same scene", "[editor][edit][save]") {
    SaveFixture fixture{"saveSceneRoundTrip"};
    const Entity parent = fixture.edit.createNamed("Parent");
    const Entity child = fixture.edit.createNamed("Child", parent);
    fixture.edit.createNamed("Second root");
    InspectedSample sample;
    sample.count = 42;
    sample.label = "Edited";
    sample.target = parent;
    sample.asset = TechEngine::AssetRef<InspectedAsset>{ResourceId{0x77}, {}};
    fixture.edit.scene.addComponent<InspectedSample>(child, sample);
    fixture.edit.session.recordSceneChange();
    fixture.edit.session.setSceneFile({SAVED_SCENE_ID, std::string(SAVED_SCENE_PATH)});
    const std::vector<std::byte> stale = {std::byte{1}, std::byte{2}, std::byte{3}};
    REQUIRE(fixture.files.write(SAVED_SCENE_PATH, stale) == std::error_code{});

    fixture.save();

    std::vector<std::byte> bytes;
    REQUIRE(fixture.files.read(SAVED_SCENE_PATH, bytes) == std::error_code{});
    std::vector<std::byte> expected;
    SceneAsset{SAVED_SCENE_ID}.write(fixture.edit.scene, expected);
    CHECK(bytes == expected);
    CHECK(fixture.fileCount() == 1);

    EditTestScene reloaded;
    REQUIRE(SceneAsset{SAVED_SCENE_ID}.read(bytes, reloaded.scene) == std::error_code{});
    const EditorSnapshot before = fixture.edit.snapshot();
    const EditorSnapshot after = reloaded.snapshot();
    REQUIRE(after.hierarchy.size() == before.hierarchy.size());
    for (std::size_t i = 0; i < before.hierarchy.size(); i++) {
        CHECK(after.hierarchy[i].name == before.hierarchy[i].name);
        CHECK(after.hierarchy[i].depth == before.hierarchy[i].depth);
        CHECK(after.hierarchy[i].childCount == before.hierarchy[i].childCount);
    }

    const std::size_t childRow = TechEngineTests::rowIndex(before, child);
    const Entity reloadedChild = after.hierarchy[childRow].entity;
    const Entity reloadedParent = after.hierarchy[TechEngineTests::rowIndex(before, parent)].entity;
    REQUIRE(reloaded.scene.hasComponent<InspectedSample>(reloadedChild));
    const InspectedSample& loaded = reloaded.scene.getComponent<InspectedSample>(reloadedChild);
    CHECK(loaded.count == 42);
    CHECK(loaded.label == "Edited");
    CHECK(loaded.target == reloadedParent);
    CHECK(loaded.asset.id == ResourceId{0x77});
}

TEST_CASE("the saved revision follows a save", "[editor][edit][save]") {
    SaveFixture fixture{"saveSceneRevision"};
    fixture.edit.createNamed("A");
    fixture.edit.session.setSceneFile({SAVED_SCENE_ID, std::string(SAVED_SCENE_PATH)});
    fixture.edit.session.recordSceneChange();
    const EditorSnapshot edited = fixture.edit.snapshot();
    REQUIRE(edited.hasUnsavedEdits());

    fixture.save();

    const EditorSnapshot saved = fixture.edit.snapshot();
    CHECK(saved.savedRevision == edited.editRevision);
    CHECK(saved.editRevision == edited.editRevision);
    CHECK_FALSE(saved.hasUnsavedEdits());

    SECTION("an edit after the save raises the marker, and the next save clears it") {
        fixture.edit.session.recordSceneChange();
        REQUIRE(fixture.edit.snapshot().hasUnsavedEdits());

        fixture.save();

        CHECK_FALSE(fixture.edit.snapshot().hasUnsavedEdits());
    }
}

TEST_CASE("a failed write leaves the marker and writes nothing", "[editor][edit][save]") {
    SaveFixture fixture{"saveSceneFailedWrite"};
    fixture.edit.createNamed("A");
    fixture.edit.session.setSceneFile({SAVED_SCENE_ID, std::string(UNMOUNTED_SCENE_PATH)});
    fixture.edit.session.recordSceneChange();
    const EditorSnapshot edited = fixture.edit.snapshot();

    SECTION("a plain save") {
        fixture.save();
    }

    SECTION("a save and close, which also keeps the app running") {
        fixture.save(AfterSave::Close);

        CHECK_FALSE(fixture.app.isStopRequested());
    }

    const EditorSnapshot after = fixture.edit.snapshot();
    CHECK(after.savedRevision == edited.savedRevision);
    CHECK(after.editRevision == edited.editRevision);
    CHECK(after.hasUnsavedEdits());
    CHECK(fixture.fileCount() == 0);
}

TEST_CASE("a session with no scene file refuses the save and writes nothing", "[editor][edit][save]") {
    SaveFixture fixture{"saveSceneNoSceneFile"};
    fixture.edit.createNamed("A");
    fixture.edit.session.recordSceneChange();
    const EditorSnapshot edited = fixture.edit.snapshot();

    fixture.save(AfterSave::Close);

    const EditorSnapshot after = fixture.edit.snapshot();
    CHECK(after.savedRevision == edited.savedRevision);
    CHECK(after.hasUnsavedEdits());
    CHECK(fixture.fileCount() == 0);
    CHECK_FALSE(fixture.app.isStopRequested());
}

TEST_CASE("only a save and close that wrote the file stops the app", "[editor][edit][save]") {
    SaveFixture fixture{"saveSceneAndClose"};
    fixture.edit.createNamed("A");
    fixture.edit.session.setSceneFile({SAVED_SCENE_ID, std::string(SAVED_SCENE_PATH)});
    fixture.edit.session.recordSceneChange();

    SECTION("a plain save keeps the app running") {
        fixture.save();

        CHECK_FALSE(fixture.app.isStopRequested());
        CHECK(fixture.fileCount() == 1);
    }

    SECTION("a save and close writes the file, then requests the stop") {
        fixture.save(AfterSave::Close);

        CHECK(fixture.app.isStopRequested());
        CHECK(fixture.fileCount() == 1);
        CHECK_FALSE(fixture.edit.snapshot().hasUnsavedEdits());
    }
}
