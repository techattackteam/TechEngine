#pragma once
#include <functional>

namespace TechEngine {
    class Application {
    protected:
        bool m_running = false;
        std::function<void()> m_runFunction;

    public:
        virtual ~Application() = default;

        void run();

        virtual void init() = 0;

        virtual void start() = 0;

        virtual void update() = 0;

        virtual void fixedUpdate() = 0;

        virtual void stop() = 0;

        virtual void destroy() = 0;
    };

    Application* createApp();
}
