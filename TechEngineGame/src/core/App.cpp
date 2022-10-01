#include "App.hpp"

namespace TechEngine {
    App::App() : TechEngineCore::App() {
/*        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](TechEngineCore::Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });*/

        timer.init();
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
            scene.fixedUpdate();
            onFixedUpdate();
            timer.updateTicks();
            accumulator -= deltaTick;
            //}
            float alpha = accumulator / deltaTick;

            scene.update();
            onUpdate();
            //panelsManager.update();
        }
    }

/*    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        if (event->getPanel()->isMainPanel()) {
            running = false;
        }
        panelsManager.unregisterAllPanels();
    }*/
}
