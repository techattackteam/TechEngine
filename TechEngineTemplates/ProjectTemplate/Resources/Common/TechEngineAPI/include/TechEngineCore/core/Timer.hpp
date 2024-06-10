#pragma once

#include <chrono>

namespace TechEngine {
    class Timer {
    private:
        inline static Timer *instance;
        std::chrono::time_point<std::chrono::system_clock> startTime;

        double lastLoopTime = 0;
        float timeCount = 0.0f;

        float deltaTime = 0.0f;
        const float TPS = 1.0f / 60.0f; // 60 ticks
        float accumulator = 0;
        float interpolation = 0;

        int fps = 0;
        int fpsCount = 0;

        int ticks = 0;
        int ticksCount = 0;
    public:
        Timer();

        void init();

        int getFPS() const;

        void updateFPS();

        void updateTicks();

        void update();

        void updateInterpolation();

        int getTicks() const;

        double getTime();

        double getDeltaTime();

        void addAccumulator(float value);

        double getLastLoopTime();

        static Timer &getInstance();

        float getAccumulator();

        const float getTPS();

        float getInterpolation();
    };
}
