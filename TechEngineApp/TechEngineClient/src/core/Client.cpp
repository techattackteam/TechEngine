#include "Client.hpp"

#include "core/Logger.hpp"
#include "script/ScriptRegister.hpp"
#include "script/ScriptEngine.hpp"
#include "network/NetworkEngine.hpp"

namespace TechEngine {
    Client::Client(Window& window) : window(window),
                                     api(systemsRegistry), AppCore() {
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
        systemsRegistry.getSystem<EventDispatcher>().subscribe(WindowCloseEvent::eventType, [this](Event* event) {
            onWindowCloseEvent((WindowCloseEvent*)(event));
        });
        //systemsRegistry.getSystem<SceneManager>().init();
        //systemsRegistry.getSystem<NetworkEngine>().init();
        //api.init();
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
