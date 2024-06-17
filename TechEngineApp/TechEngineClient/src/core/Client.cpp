#include "Client.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    Client::Client(Window& window) : window(window),
                                     renderer(),
                                     networkEngine(eventDispatcher, sceneManager),
                                     api(&sceneManager, &materialManager, &eventDispatcher, &networkEngine)  {
        Logger::init("TechEngineClient");
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event* event) {
            onWindowCloseEvent((WindowCloseEvent*)(event));
        });
        physicsEngine.init();
        ScriptRegister::getInstance()->init(&scriptEngine);
    }

    Client::~Client() = default;

    void Client::onFixedUpdate() {
        eventDispatcher.fixedSyncEventManager.execute();
        scriptEngine.onFixedUpdate();
        sceneManager.getScene().fixedUpdate();
        networkEngine.fixedUpdate();
    }

    void Client::onUpdate() {
        sceneManager.getScene().update();
        scriptEngine.onUpdate();
        eventDispatcher.syncEventManager.execute();
        networkEngine.update();
        window.onUpdate();
    }


    /*void Client::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.fixedSyncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                sceneManager.getScene().fixedUpdate();
                networkEngine.fixedUpdate();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }

            sceneManager.getScene().update();
            networkEngine.update();
            timer.updateInterpolation();
            ScriptEngine::getInstance()->onUpdate();
            eventDispatcher.syncEventManager.execute();
            onUpdate();
            timer.update();
            timer.updateFPS();
        }
    }*/


    void Client::onWindowCloseEvent(WindowCloseEvent* event) {
        running = false;
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return nullptr;
}
