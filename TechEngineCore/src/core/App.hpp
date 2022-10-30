#pragma once

#include "../stateMachine/StateMachineManager.hpp"
#include "../event/EventDispatcher.hpp"
#include "Timer.hpp"

namespace TechEngineCore {
    class App {
    protected:
        bool running = true;

    public:
        EventDispatcher eventDispatcher;
        StateMachineManager stateMachineManager;
        Timer timer;

        App();

        virtual ~App();

        virtual void run() = 0;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:
        void onAppCloseRequestEvent();
    };

    App *createApp();
};
