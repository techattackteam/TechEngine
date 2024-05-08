#include "AppCore.hpp"

#include "Logger.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    AppCore::AppCore() : scriptEngine(false),
                         materialManager(eventDispatcher, textureManager),
                         sceneManager(eventDispatcher, physicsEngine, materialManager, textureManager, filePaths),
                         physicsEngine(eventDispatcher, sceneManager.getScene()),
                         api(&sceneManager, &eventDispatcher, &materialManager) {
        eventDispatcher.subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
            onAppCloseRequestEvent();
        });
        timer.init();
    }

    AppCore::~AppCore() {
    }

    void AppCore::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }
            timer.updateInterpolation();
            onUpdate();
            timer.update();
            timer.updateFPS();
        }
    }

    void AppCore::onAppCloseRequestEvent() {
        running = false;
    }
}
