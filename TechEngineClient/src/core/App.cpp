#include "App.hpp"
#include "script/ScriptEngine.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    App::App() : TechEngine::AppCore() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        timer.init();
    }

    App::~App() = default;

    void App::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.fixedSyncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                scene.fixedUpdate();
                onFixedUpdate();
                timer.addAccumulator(-timer.getTPS());
            }

            scene.update();
            timer.updateInterpolation();
            stateMachineManager.update();
            ScriptEngine::getInstance()->onUpdate();
            onUpdate();
            eventDispatcher.syncEventManager.execute();
            timer.update();
            timer.updateFPS();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        running = false;
    }
}