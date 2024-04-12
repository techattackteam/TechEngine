#include "AppCore.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    AppCore::AppCore() : timer(), textureManager(),
                         materialManager(textureManager),
                         sceneManager(projectManager, physicsEngine, materialManager, textureManager),
                         projectManager(sceneManager, textureManager, materialManager), physicsEngine(sceneManager.getScene()),
                         api(&sceneManager, &eventDispatcher, &materialManager) {
        ScriptEngine* scriptEngine = new ScriptEngine(false);
        eventDispatcher.subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
            onAppCloseRequestEvent();
        });
    }

    AppCore::~AppCore() {
    }

    void AppCore::onAppCloseRequestEvent() {
        running = false;
    }
}
