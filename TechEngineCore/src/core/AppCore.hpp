#pragma once

#include "../stateMachine/StateMachineManager.hpp"
#include "../eventSystem/EventDispatcher.hpp"
#include "Timer.hpp"

namespace TechEngine {
    class AppCore {
    protected:
        bool running = true;

    public:
        EventDispatcher eventDispatcher;
        StateMachineManager stateMachineManager;
        Timer timer;

        AppCore();

        virtual ~AppCore();

        virtual void run() = 0;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:
        void onAppCloseRequestEvent();
    };

    AppCore* createApp();
};
