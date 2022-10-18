#include "App.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    App::App() : TechEngineCore::App() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](TechEngineCore::Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        timer.init();
        ScriptEngine::getInstance()->onStart();
    }

    App::~App() {
    }

    void App::run() {
        while (running) {
            timer.addAccumulator(timer.getDeltaTime());
            stateMachineManager.update();
            while (timer.getAccumulator() >= timer.getTPS()) {
                timer.updateTicks();

                eventDispatcher.syncEventManager.execute();
                ScriptEngine::getInstance()->onFixedUpdate();
                scene.fixedUpdate();
                onFixedUpdate();

                timer.addAccumulator(-timer.getTPS());
            }

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
