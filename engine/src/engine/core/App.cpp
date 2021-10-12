#include <iostream>
#include "App.hpp"

namespace Engine {

    Engine::App::App() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        timer.init();
    }

    Engine::App::~App() {

    }

    void App::run() {
        float deltaTick = 1.0f / 20.0f;
        float accumulator = 0;
        while (running) {
            float deltaFrame = timer.getDeltaTime();
            accumulator += deltaFrame;
            stateMachineManager.update();
            while (accumulator >= deltaTick) {
                eventDispatcher.syncEventManager.execute();
                scene.fixedUpdate();
                onFixedUpdate();
                timer.updateTicks();
                accumulator -= deltaTick;
            }
            float alpha = accumulator / deltaTick;

            onUpdate();
            scene.update();
            panelsManager.update();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        std::cout << "Window closed" << std::endl;
        if (event->getPanel()->isMainPanel()) {
            running = false;
        }
        panelsManager.unregisterAllPanels();
    }
}
