#include "Timer.hpp"

namespace TechEngineCore {

    Timer::Timer() {
        Timer::instance = this;
    }

    void Timer::init() {
        startTime = std::chrono::system_clock::now();
        lastLoopTime = getTime();
    }

    int Timer::getFPS() const {
        if (fps > 0) {
            return fps;
        } else {
            return fpsCount;
        }
    }

    void Timer::updateFPS() {
        fpsCount++;
    }

    void Timer::updateTicks() {
        ticksCount++;
    }

    void Timer::update() {
        if (timeCount > 1.0f) {
            fps = fpsCount;
            fpsCount = 0;

            ticks = ticksCount;
            ticksCount = 0;

            timeCount -= 1.0f;
        }
    }

    int Timer::getTicks() const {
        return ticks;
    }

    double Timer::getDeltaTime() {
        double time = getTime();
        float delta = (float) (time - lastLoopTime);
        lastLoopTime = time;
        timeCount += delta;
        return delta;
    }

    double Timer::getTime() {
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> secondsElapsed = now - startTime;
        return secondsElapsed.count();
    }

    double Timer::getLastLoopTime() {
        return lastLoopTime;
    }

    Timer &Timer::getInstance() {
        return *instance;
    }
}
