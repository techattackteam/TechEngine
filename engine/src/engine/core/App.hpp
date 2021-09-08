
#include "../event/EventDispatcher.hpp"
#include "../stateMachine/StateMachine.hpp"
#include "../stateMachine/StateMachineManager.hpp"
#include "../event/events/window/WindowCloseEvent.hpp"
#include "../panels/PanelsManager.hpp"
#include "../panels/ImGuiPanel.hpp"
#include "../panels/RendererPanel.hpp"

#pragma once

namespace Engine {
    class App {
    private:
        bool running = true;

        unsigned int vertexArray, vertexBuffer, indexBuffer;
    public:
        EventDispatcher eventDispatcher{};
        StateMachineManager stateMachineManager{};
        PanelsManager panelsManager{};

        App();

        virtual ~App();

        void run();

        virtual void onUpdate() = 0;

    private:

        void onWindowCloseEvent(Engine::WindowCloseEvent *event);
    };

    App *createApp();

}

