#include "Timer.hpp"

namespace TechEngine {

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

    int Timer::getTicks() const {
        return ticks;
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

    void Timer::addAccumulator(float value) {
        accumulator += value;
    }

    float Timer::getAccumulator() {
        return accumulator;
    }

    Timer &Timer::getInstance() {
        return *instance;
    }

    const float Timer::getTPS() {
        return TPS;
    }

    void Timer::updateInterpolation() {
        interpolation = getAccumulator() / getTPS();
    }

    float Timer::getInterpolation() {
        return interpolation;
    }


}
