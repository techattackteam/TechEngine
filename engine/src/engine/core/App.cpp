#include <iostream>
#include "App.hpp"

namespace Engine {

    Engine::App::App() {
        eventDispatcher.subscribe(WindowCloseEvent::eventType, [this](Event *event) {
            onWindowCloseEvent((WindowCloseEvent *) (event));
        });

        panel2 = new ImGuiPanel("Test");
        panel = new ImGuiPanel("Test");
    }

    Engine::App::~App() {

    }

    void App::run() {
        while (running) {
            eventDispatcher.syncEventManager.execute();
            stateMachineManager.update();
            onUpdate();


            panelsManager.update();
        }
    }

    void App::onWindowCloseEvent(WindowCloseEvent *event) {
        std::cout << "Window closed" << std::endl;
        if (event->getPanel()->isMainPanel()) {
            running = false;
        }
        panelsManager.unregisterPanel(event->getPanel());
    }
}
