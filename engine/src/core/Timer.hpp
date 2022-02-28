#pragma once

#include <chrono>

namespace Engine {
    class Timer {
    private:
        inline static Timer *instance;
        std::chrono::time_point<std::chrono::system_clock> startTime;

        double lastLoopTime{};
        float timeCount{};

        float deltaTime{};

        int fps{};
        int fpsCount{};

        int ticks{};
        int ticksCount{};

    public:
        Timer();

        void init();

        int getFPS() const;

        void updateFPS();

        void updateTicks();

        void update();

        int getTicks() const;

        double getTime();

        double getDeltaTime();

        double getLastLoopTime();

        static Timer &getInstance();

    };
}
