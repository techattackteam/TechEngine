#include "App.hpp"

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
        float deltaTick = 1.0f / 20.0f;
        float accumulator = 0;
        while (running) {
            float deltaFrame = timer.getDeltaTime();
            accumulator += deltaFrame;
            stateMachineManager.update();
            //while (accumulator >= deltaTick) {
            eventDispatcher.syncEventManager.execute();
            timer.updateTicks();
            ScriptEngine::getInstance()->onFixedUpdate();
            scene.fixedUpdate();
            onFixedUpdate();


            accumulator -= deltaTick;
            //}
            float alpha = accumulator / deltaTick;
            ScriptEngine::getInstance()->onUpdate();
            scene.update();
            onUpdate();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        running = false;
    }
}
