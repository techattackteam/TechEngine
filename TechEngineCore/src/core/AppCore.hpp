#pragma once

#include "Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    class AppCore {
    protected:
        bool running = true;

    public:
        EventDispatcher eventDispatcher;
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
