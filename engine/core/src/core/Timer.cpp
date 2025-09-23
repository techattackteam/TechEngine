#include "Timer.hpp"

#include "Logger.hpp"

namespace TechEngine {
    void Timer::init() {
        m_frameTime = 0.0f;
        m_frameTimeMs = 0.0f;
        m_avgFrameTimeMs = 0.0f;
        m_avgFps = 0.0f;
        m_timeSinceFpsUpdate = 0.0f;
        m_framesSinceFpsUpdate = 0;
        m_fps = 0;
        m_accumulator = 0.0f;
        m_interpolation = 0.0f;
        m_ticks = 0;
        m_ticksCount = 0;
    }

    void Timer::onStart() {
        m_startTime = std::chrono::high_resolution_clock::now();
        m_lastFrameTime = m_startTime;
    }

    void Timer::shutdown() {
    }

    void Timer::tick(float deltaTime) {
        m_frameTime = deltaTime;
        auto currentTime = std::chrono::high_resolution_clock::now();
        m_lastFrameTime = currentTime;

        m_frameTimeMs = m_frameTime * 1000.0f;
        float currentFps = m_frameTime > 0.0f ? 1.0f / m_frameTime : 0.0f;

        m_avgFps = (m_avgFps * 0.99f) + (currentFps * 0.01f);
        m_avgFrameTimeMs = (m_avgFrameTimeMs * 0.99f) + (m_frameTimeMs * 0.01f);

        m_timeSinceFpsUpdate += m_frameTimeMs;
        m_framesSinceFpsUpdate++;

        if (m_timeSinceFpsUpdate >= 1000.0f) {
            m_fps = m_framesSinceFpsUpdate;
            m_framesSinceFpsUpdate = 0;
            m_timeSinceFpsUpdate = 0.0f;

            // This logic for ticks should also be inside the loop
            m_ticks = m_ticksCount;
            m_ticksCount = 0;
        }
    }

    void Timer::tick() {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> deltaTime = currentTime - m_lastFrameTime;
        tick(deltaTime.count());
    }

    int Timer::getFPS() const {
        return m_fps;
    }

    float Timer::getFrameTimeMS() const {
        return m_frameTimeMs;
    }

    float Timer::getAverageFPS() const {
        return m_avgFps;
    }

    float Timer::getAverageFrameTimeMS() const {
        return m_avgFrameTimeMs;
    }

    int Timer::getTicks() const {
        return m_ticks;
    }

    float Timer::getDeltaTime() const {
        return m_frameTime;
    }

    float Timer::getAccumulator() const {
        return m_accumulator;
    }

    float Timer::getTPS() const {
        return TICKS_PER_SECOND;
    }

    float Timer::getInterpolation() const {
        return m_interpolation;
    }

    void Timer::addAccumulator(float value) {
        m_accumulator += value;
    }

    void Timer::updateInterpolation() {
        m_interpolation = m_accumulator / m_ticksCount;
    }

    void Timer::updateTicks() {
        m_ticks++;
    }
}
