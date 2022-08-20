#pragma once

#include <TechEngineCore.hpp>

#include "../event/EventDispatcher.hpp"
#include "../stateMachine/StateMachineManager.hpp"
#include "../panels/PanelsManager.hpp"
#include "../scene/Scene.hpp"
#include "../events/window/WindowCloseEvent.hpp"

namespace TechEngine {
    class Engine_API App : public TechEngineCore::App {
    private:
        bool running = true;

    public:
        TechEngineCore::EventDispatcher eventDispatcher;
        TechEngineCore::StateMachineManager stateMachineManager;
        PanelsManager panelsManager;
        Scene scene;

        App();

        ~App() override;

        void run() override;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:

        void onWindowCloseEvent(TechEngine::WindowCloseEvent *event);
    };

    App *createApp();

}

