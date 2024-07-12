#include "Client.hpp"

#include "core/FileSystem.hpp"
#include "core/Logger.hpp"
#include "script/ScriptRegister.hpp"
#include "script/ScriptEngine.hpp"
#include "network/NetworkEngine.hpp"
#include "physics/PhysicsEngine.hpp"
#include "texture/TextureManager.hpp"

namespace TechEngine {
    Client::Client(Window& window) : window(window),
                                     project(systemsRegistry),
                                     api(systemsRegistry),
                                     AppCore() {
        systemsRegistry.registerSystem<Renderer>();
        systemsRegistry.registerSystem<NetworkEngine>(systemsRegistry);
    }

    Client::~Client() = default;

    void Client::init() {
        AppCore::init();
        systemsRegistry.getSystem<Logger>().init("TechEngineClient");
        systemsRegistry.getSystem<PhysicsEngine>().init();
        ScriptRegister::getInstance()->init(&systemsRegistry.getSystem<ScriptEngine>());
        api.init();
        std::vector<std::string> paths = {
            project.getResourcesPath().string(),
            project.getCommonResourcesPath().string(),
            project.getAssetsPath().string(),
            project.getCommonAssetsPath().string()
        };
        systemsRegistry.getSystem<TextureManager>().init(FileSystem::getAllFilesWithExtension(paths, {".jpg", ".png"}, true));
        systemsRegistry.getSystem<MaterialManager>().init(FileSystem::getAllFilesWithExtension(paths, {".mat"}, true));
        systemsRegistry.getSystem<SceneManager>().init(FileSystem::getAllFilesWithExtension(paths, {".scene"}, true));
        systemsRegistry.getSystem<EventDispatcher>().subscribe(WindowCloseEvent::eventType, [this](Event* event) {
            onWindowCloseEvent((WindowCloseEvent*)(event));
        });
    }

    void Client::onFixedUpdate() {
        systemsRegistry.getSystem<EventDispatcher>().fixedSyncEventManager.execute();
        systemsRegistry.getSystem<ScriptEngine>().onFixedUpdate();
        systemsRegistry.getSystem<SceneManager>().getScene().fixedUpdate();
        systemsRegistry.getSystem<NetworkEngine>().fixedUpdate();
    }

    void Client::onUpdate() {
        systemsRegistry.getSystem<EventDispatcher>().syncEventManager.execute();
        systemsRegistry.getSystem<ScriptEngine>().onUpdate();
        systemsRegistry.getSystem<SceneManager>().getScene().update();
        systemsRegistry.getSystem<NetworkEngine>().update();
        window.onUpdate();
    }

    void Client::onWindowCloseEvent(WindowCloseEvent* event) {
        running = false;
    }
}
