#pragma once

#include "systems/System.hpp"
#include <chrono>

namespace TechEngine {
    class CORE_DLL Timer : public System {
    private:
        // Use high_resolution_clock for stable interval measurements
        std::chrono::time_point<std::chrono::high_resolution_clock> m_startTime;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_lastFrameTime;

        // Per-second FPS calculation
        float m_timeSinceFpsUpdate = 0.0f;
        int m_framesSinceFpsUpdate = 0;
        int m_fps = 0;

        // Instantaneous and Averaged Metrics
        float m_frameTime = 0.0f; // Time for the last frame, in seconds
        float m_frameTimeMs = 0.0f; // Time for the last frame, in milliseconds
        float m_avgFrameTimeMs = 0.0f; // Exponential moving average for frame time
        float m_avgFps = 0.0f; // Exponential moving average for FPS

        // Game loop (fixed timestep) logic - Unchanged
        const float TICKS_PER_SECOND = 1.0f / 60.0f; // 60 ticks
        float m_accumulator = 0.0f;
        float m_interpolation = 0.0f;
        int m_ticks = 0;
        int m_ticksCount = 0;

    public:
        void init() override;

        void onStart() override;

        void shutdown() override;

        void tick(float deltaTime);

        void tick();

        int getFPS() const;

        float getFrameTimeMS() const;

        float getAverageFPS() const;

        float getAverageFrameTimeMS() const;

        int getTicks() const;

        float getDeltaTime() const;

        float getAccumulator() const;

        float getTPS() const;

        float getInterpolation() const;

        void addAccumulator(float value);

        void updateInterpolation();

        void updateTicks();
    };
}
