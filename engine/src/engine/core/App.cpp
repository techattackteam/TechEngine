#include <iostream>
#include "App.hpp"

namespace Engine {

    Engine::App::App() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        new RendererPanel();
    }

    Engine::App::~App() {

    }

    void App::run() {
        while (running) {
            eventDispatcher.syncEventManager.execute();
            stateMachineManager.update();
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
