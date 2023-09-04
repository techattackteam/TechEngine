#include "App.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    App::App() : TechEngine::AppCore() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        timer.init();
        ScriptEngine::getInstance()->onStart();
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

            stateMachineManager.update();
            eventDispatcher.syncEventManager.execute();
            timer.updateInterpolation();
            ScriptEngine::getInstance()->onUpdate();
            scene.update();
            onUpdate();
            timer.update();
            timer.updateFPS();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        running = false;
    }
}