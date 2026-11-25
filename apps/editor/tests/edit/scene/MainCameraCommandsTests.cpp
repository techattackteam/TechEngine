#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/testing/LogCapture.hpp>

#include <edit/fields/FieldValue.hpp>
#include <edit/scene/InspectorCommands.hpp>
#include <edit/session/EditRecord.hpp>
#include <edit/session/EditSession.hpp>
#include <edit/session/EditTestScene.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

using TechEngine::AddComponentCommand;
using TechEngine::Camera;
using TechEngine::ComponentRegistry;
using TechEngine::ComponentTypeId;
using TechEngine::EditRecord;
using TechEngine::Entity;
using TechEngine::FieldEditRecord;
using TechEngine::FieldValue;
using TechEngine::Scene;
using TechEngine::SetFieldCommand;
using TechEngineTests::findRecord;

// main's position in Camera's visit.
static constexpr std::uint32_t CAMERA_MAIN_FIELD = 4;

static ComponentRegistry& registerCameraEditComponents(ComponentRegistry& registry) {
    registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
    registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
    registry.registerComponent<TechEngine::Name>(TechEngine::Name::tag);
    registry.registerComponent<Camera>(Camera::tag);
    return registry;
}

static ComponentTypeId cameraType() {
    return TechEngine::componentTypeId<Camera>();
}

struct CameraEditScene {
    ComponentRegistry registry;
    Scene scene;
    TechEngine::EditSession session;

    CameraEditScene() : scene(registerCameraEditComponents(registry)) {
    }

    Entity createCamera(const bool main) {
        const Entity entity = scene.createEntity();
        Camera camera;
        camera.main = main;
        scene.addComponent<Camera>(entity, camera);
        return entity;
    }

    void setMain(std::vector<Entity> entities, const bool main) {
        SetFieldCommand command{session, registry, std::move(entities), cameraType(), CAMERA_MAIN_FIELD, FieldValue{main}};
        command.apply(scene);
    }

    void addCamera(std::vector<Entity> entities) {
        AddComponentCommand command{session, std::move(entities), cameraType()};
        command.apply(scene);
    }

    bool isMain(const Entity entity) const {
        return scene.getComponent<Camera>(entity).main;
    }
};

TEST_CASE("ticking main on a camera turns the main one off and records both in the same edit", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity previous = edit.createCamera(true);
    const Entity next = edit.createCamera(false);
    const Entity other = edit.createCamera(false);

    edit.setMain({next}, true);

    CHECK(edit.isMain(next));
    CHECK_FALSE(edit.isMain(previous));
    CHECK_FALSE(edit.isMain(other));
    const std::span<const EditRecord> records = edit.session.getRecords();
    REQUIRE(records.size() == 2);
    const FieldEditRecord* turnedOn = findRecord<FieldEditRecord>(records, next);
    REQUIRE(turnedOn != nullptr);
    CHECK(turnedOn->component == cameraType());
    CHECK(turnedOn->field == CAMERA_MAIN_FIELD);
    CHECK(turnedOn->oldValue == FieldValue{false});
    const FieldEditRecord* turnedOff = findRecord<FieldEditRecord>(records, previous);
    REQUIRE(turnedOff != nullptr);
    CHECK(turnedOff->component == cameraType());
    CHECK(turnedOff->field == CAMERA_MAIN_FIELD);
    CHECK(turnedOff->oldValue == FieldValue{true});
    CHECK(findRecord<FieldEditRecord>(records, other) == nullptr);
}

TEST_CASE("ticking main on the camera that is already main records nothing", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity mainCamera = edit.createCamera(true);
    edit.createCamera(false);

    edit.setMain({mainCamera}, true);

    CHECK(edit.isMain(mainCamera));
    CHECK(edit.session.getRecords().empty());
}

TEST_CASE("ticking main on several selected cameras is refused with a warning and changes nothing", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity mainCamera = edit.createCamera(true);
    const Entity first = edit.createCamera(false);
    const Entity second = edit.createCamera(false);
    const TechEngineTests::LogCaptureGuard warnings{TechEngine::Level::Warn};

    edit.setMain({first, second}, true);

    CHECK(edit.isMain(mainCamera));
    CHECK_FALSE(edit.isMain(first));
    CHECK_FALSE(edit.isMain(second));
    CHECK(edit.session.getRecords().empty());
    CHECK(warnings.getMessages().size() == 1);
}

TEST_CASE("unticking main on the main camera leaves the scene with none", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity mainCamera = edit.createCamera(true);

    edit.setMain({mainCamera}, false);

    CHECK_FALSE(edit.isMain(mainCamera));
    CHECK(TechEngine::Camera::countMainCameras(edit.scene) == 0);
    REQUIRE(edit.session.getRecords().size() == 1);
    const FieldEditRecord* record = findRecord<FieldEditRecord>(edit.session.getRecords(), mainCamera);
    REQUIRE(record != nullptr);
    CHECK(record->oldValue == FieldValue{true});
}

TEST_CASE("the first Camera added to a scene is main", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity entity = edit.scene.createEntity();

    edit.addCamera({entity});

    REQUIRE(edit.scene.hasComponent<Camera>(entity));
    CHECK(edit.isMain(entity));
}

TEST_CASE("a Camera added to a scene that already has a main camera starts with main off", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const Entity mainCamera = edit.createCamera(true);
    const Entity entity = edit.scene.createEntity();

    edit.addCamera({entity});

    REQUIRE(edit.scene.hasComponent<Camera>(entity));
    CHECK_FALSE(edit.isMain(entity));
    CHECK(edit.isMain(mainCamera));
}

TEST_CASE("adding Cameras to several entities at once leaves exactly one main, the first added", "[editor][edit][commands][camera]") {
    CameraEditScene edit;
    const std::vector<Entity> entities{edit.scene.createEntity(), edit.scene.createEntity(), edit.scene.createEntity()};

    edit.addCamera(entities);

    CHECK(TechEngine::Camera::countMainCameras(edit.scene) == 1);
    CHECK(edit.isMain(entities[0]));
    CHECK_FALSE(edit.isMain(entities[1]));
    CHECK_FALSE(edit.isMain(entities[2]));
}
