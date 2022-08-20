#include "App.hpp"

namespace TechEngineServer {
    App::App() : networkHandler(std::string("localhost"), 25565), TechEngineCore::App() {
        timer.init();
        networkHandler.init();
    }

    App::~App() {

    }

    void App::run() {
        float deltaTick = 1.0f / 20.0f;
        float accumulator = 0;
        while (running) {
            float deltaFrame = timer.getDeltaTime();
            accumulator += deltaFrame;
            //while (accumulator >= deltaTick) {
            onFixedUpdate();
            timer.updateTicks();
            accumulator -= deltaTick;
            eventDispatcher.syncEventManager.execute();
            //}
            float alpha = accumulator / deltaTick;
            networkHandler.update();
            eventDispatcher.asyncEventManager.execute();
            onUpdate();
        }
    }
}
