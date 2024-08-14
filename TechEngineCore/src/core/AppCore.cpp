#include "AppCore.hpp"

#include "Logger.hpp"
#include "Timer.hpp"
#include "events/appManagement/AppCloseRequestEvent.hpp"
#include "mesh/MeshManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "script/ScriptEngine.hpp"
#include "system/SystemsRegistry.hpp"
#include "texture/TextureManager.hpp"

namespace TechEngine {
    AppCore::AppCore() {
        systemsRegistry.registerSystem<Timer>();
        systemsRegistry.registerSystem<Logger>();
        systemsRegistry.registerSystem<EventDispatcher>();
        systemsRegistry.registerSystem<MeshManager>();
        systemsRegistry.registerSystem<TextureManager>();
        systemsRegistry.registerSystem<MaterialManager>(systemsRegistry);
        //systemsRegistry.registerSystem<PhysicsEngine>(systemsRegistry);
        systemsRegistry.registerSystem<ScriptEngine>();
        systemsRegistry.registerSystem<SceneManager>(systemsRegistry);
    }

    void AppCore::init() {
        systemsRegistry.getSystem<Timer>().init();
        systemsRegistry.getSystem<MeshManager>().init();
        systemsRegistry.getSystem<EventDispatcher>().subscribe(AppCloseRequestEvent::eventType, [this](Event* event) {
            onAppCloseRequestEvent();
        });
    }

    AppCore::~AppCore() {
    }

    void AppCore::run() {
        Timer& timer = systemsRegistry.getSystem<Timer>();
        systemsRegistry.getSystem<Timer>().init();
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
        TE_LOGGER_INFO("App close request event received");
    }
}
