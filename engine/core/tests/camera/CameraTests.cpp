#include <TechEngine/core/camera/Camera.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/testing/LogCapture.hpp>

#include <scene/SceneTestRegistry.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string>
#include <system_error>
#include <vector>

using TechEngine::Camera;
using TechEngine::ComponentRegistry;
using TechEngine::Entity;
using TechEngine::ResourceId;
using TechEngine::Scene;
using TechEngine::SceneAsset;

static constexpr ResourceId CAMERA_SCENE_ID{0x5EC0FFEE0000CA01};
static constexpr const char* NO_CAMERA_WARNING = "No entity has a Camera with main set";

static ComponentRegistry& registerCameraComponents(ComponentRegistry& registry) {
    TechEngineTests::registerBuiltInSceneComponents(registry);
    registry.registerComponent<Camera>(Camera::tag);
    return registry;
}

static Entity createChild(Scene& scene, const Entity parent) {
    const Entity child = scene.createEntity();
    const bool parented = scene.setParent(child, parent, scene.getChildren(parent).size());
    REQUIRE(parented);
    return child;
}

static void addCamera(Scene& scene, const Entity entity, const bool main) {
    Camera camera;
    camera.main = main;
    scene.addComponent<Camera>(entity, camera);
}

static std::size_t countNoCameraWarnings(const TechEngineTests::LogCaptureGuard& logs) {
    std::size_t count = 0;
    for (const std::string& message: logs.getMessages()) {
        if (message.find(NO_CAMERA_WARNING) != std::string::npos) {
            count++;
        }
    }
    return count;
}

TEST_CASE("the first entity in hierarchy order whose Camera is main is the main camera", "[core][camera]") {
    ComponentRegistry registry;
    Scene scene(registerCameraComponents(registry));
    const Entity first = scene.createEntity();
    const Entity second = scene.createEntity();
    const Entity notMain = createChild(scene, first);
    const Entity deep = createChild(scene, createChild(scene, first));
    const Entity laterSibling = createChild(scene, first);
    addCamera(scene, notMain, false);
    addCamera(scene, laterSibling, true);
    addCamera(scene, second, true);

    SECTION("a later child of the first root wins over the second root") {
        Entity camera;
        bool warnedNone = false;
        CHECK(TechEngine::findMainCamera(scene, camera, warnedNone));
        CHECK(camera == laterSibling);
    }

    SECTION("a camera deeper in an earlier subtree wins over a later sibling of its ancestor") {
        addCamera(scene, deep, true);

        Entity camera;
        bool warnedNone = false;
        CHECK(TechEngine::findMainCamera(scene, camera, warnedNone));
        CHECK(camera == deep);
    }

    SECTION("a root before every other camera wins") {
        addCamera(scene, first, true);

        Entity camera;
        bool warnedNone = false;
        CHECK(TechEngine::findMainCamera(scene, camera, warnedNone));
        CHECK(camera == first);
    }
}

TEST_CASE("a scene whose only Camera is not main has no main camera", "[core][camera]") {
    ComponentRegistry registry;
    Scene scene(registerCameraComponents(registry));
    addCamera(scene, scene.createEntity(), false);
    const TechEngineTests::LogCaptureGuard logs{TechEngine::Level::Warn};

    Entity camera;
    bool warnedNone = false;
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK_FALSE(camera.valid());
}

TEST_CASE("a scene with no main camera returns none and warns once for the same flag", "[core][camera]") {
    ComponentRegistry registry;
    Scene scene(registerCameraComponents(registry));
    scene.createEntity();
    const TechEngineTests::LogCaptureGuard logs{TechEngine::Level::Warn};

    Entity camera;
    bool warnedNone = false;
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));

    CHECK_FALSE(camera.valid());
    CHECK(warnedNone);
    CHECK(countNoCameraWarnings(logs) == 1);
}

TEST_CASE("an empty scene has no main camera and warns", "[core][camera]") {
    ComponentRegistry registry;
    const Scene scene(registerCameraComponents(registry));
    const TechEngineTests::LogCaptureGuard logs{TechEngine::Level::Warn};

    Entity camera;
    bool warnedNone = false;
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK(countNoCameraWarnings(logs) == 1);
}

TEST_CASE("finding a main camera clears the flag, so a scene that loses it warns again", "[core][camera]") {
    ComponentRegistry registry;
    Scene scene(registerCameraComponents(registry));
    const Entity entity = scene.createEntity();
    const TechEngineTests::LogCaptureGuard logs{TechEngine::Level::Warn};
    Entity camera;
    bool warnedNone = false;
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));

    addCamera(scene, entity, true);
    CHECK(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK_FALSE(warnedNone);

    scene.getComponent<Camera>(entity).main = false;
    CHECK_FALSE(TechEngine::findMainCamera(scene, camera, warnedNone));
    CHECK(countNoCameraWarnings(logs) == 2);
}

TEST_CASE("a scene saves and loads every Camera field", "[core][camera][scene]") {
    Camera saved;
    saved.verticalFieldOfView = 45.0f;
    saved.nearPlane = 0.25f;
    saved.farPlane = 250.0f;
    saved.exposure = 9.5f;
    saved.main = false;
    std::vector<std::byte> bytes;
    {
        ComponentRegistry registry;
        Scene source(registerCameraComponents(registry));
        source.addComponent<Camera>(source.createEntity(), saved);
        SceneAsset{CAMERA_SCENE_ID}.write(source, bytes);
    }

    ComponentRegistry registry;
    Scene loaded(registerCameraComponents(registry));
    SceneAsset asset;
    const std::error_code readError = asset.read(bytes, loaded);
    REQUIRE(!readError);
    const std::vector<Entity> roots = loaded.getRoots();
    REQUIRE(roots.size() == 1);
    REQUIRE(loaded.hasComponent<Camera>(roots[0]));
    const Camera& camera = loaded.getComponent<Camera>(roots[0]);
    CHECK(camera.verticalFieldOfView == saved.verticalFieldOfView);
    CHECK(camera.nearPlane == saved.nearPlane);
    CHECK(camera.farPlane == saved.farPlane);
    CHECK(camera.exposure == saved.exposure);
    CHECK(camera.main == saved.main);
}
