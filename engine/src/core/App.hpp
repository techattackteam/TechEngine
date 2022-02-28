
#include "../event/EventDispatcher.hpp"
#include "../stateMachine/StateMachine.hpp"
#include "../stateMachine/StateMachineManager.hpp"
#include "../event/events/window/WindowCloseEvent.hpp"
#include "../panels/PanelsManager.hpp"
#include "../panels/CustomPanel.hpp"
#include "../panels/RendererPanel.hpp"
#include "../scene/Scene.hpp"
#include "Timer.hpp"
#include "Core.hpp"

#pragma once

namespace Engine {
    class Engine_API App {
    private:
        bool running = true;

    public:
        EventDispatcher eventDispatcher;
        StateMachineManager stateMachineManager;
        PanelsManager panelsManager;
        Scene scene;
        Timer timer;

        App();

        virtual ~App();

        void run();

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:

        void onWindowCloseEvent(Engine::WindowCloseEvent *event);
    };

    App *createApp();

}

