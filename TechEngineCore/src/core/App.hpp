#pragma once

#include "../stateMachine/StateMachineManager.hpp"
#include "../event/EventDispatcher.hpp"
#include "Timer.hpp"
#include "Core.hpp"

namespace TechEngineCore {
    class Engine_API App {
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

    };

    App *createApp();
};
