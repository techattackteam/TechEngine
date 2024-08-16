#pragma once

namespace TechEngine {
    class Application {
    protected:
        bool running = false;

    public:
        virtual ~Application() = default;

        void run();

        virtual void init() = 0;

        virtual void update() = 0;

        virtual void fixedUpdate() = 0;

        virtual void destroy() = 0;
    };

    Application* createApp();
}
