#include "Client.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    Client::Client(std::string name, int width, int height) : AppCore(),
                                                              textureManager(),
                                                              materialManager(textureManager),
                                                              sceneManager(projectManager, physicsEngine, materialManager, textureManager),
                                                              projectManager(sceneManager, textureManager, materialManager), physicsEngine(sceneManager.getScene()),
                                                              window(name, width, height),
                                                              renderer(sceneManager.getScene()),
                                                              api(&sceneManager, &eventDispatcher, &materialManager) {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event* event) {
            onWindowCloseEvent((WindowCloseEvent*)(event));
        });

        timer.init();
        physicsEngine.init();
    }

    Client::~Client() = default;

    void Client::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.fixedSyncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                sceneManager.getScene().fixedUpdate();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }

            sceneManager.getScene().update();
            timer.updateInterpolation();
            ScriptEngine::getInstance()->onUpdate();
            eventDispatcher.syncEventManager.execute();
            onUpdate();
            timer.update();
            timer.updateFPS();
        }
    }

    void Client::onWindowCloseEvent(WindowCloseEvent* event) {
        running = false;
    }
}
