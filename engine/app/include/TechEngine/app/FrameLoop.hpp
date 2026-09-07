#pragma once

#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/FrameContext.hpp>

#include <concepts>
#include <cstdint>

namespace TechEngine {
    class FrameLoop {
    public:
        static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;
        static constexpr double MAX_FRAME_DELTA_TIME = 0.25;

        FrameLoop(const EngineContext& engine, Role role, double fixedDeltaTime = FIXED_DELTA_TIME, double maxFrameDeltaTime = MAX_FRAME_DELTA_TIME);

        const FrameContext& advance(double frameDeltaTime);

        template<std::invocable<const FrameContext&> Step>
        const FrameContext& advance(double frameDeltaTime, Step&& onFixedStep) {
            TE_PROFILER_FUNCTION();

            if (frameDeltaTime < 0.0) {
                frameDeltaTime = 0.0;
            }

            const double clampedDeltaTime = frameDeltaTime > m_maxFrameDeltaTime ? m_maxFrameDeltaTime : frameDeltaTime;
            const std::uint64_t previousTick = m_frame.tick;

            m_frame.frameIndex++;
            m_frame.deltaTime = static_cast<float>(clampedDeltaTime);

            m_accumulator += clampedDeltaTime;

            {
                TE_PROFILER_SCOPE("FixedSteps");

                while (m_accumulator >= m_fixedDeltaTime) {
                    m_accumulator -= m_fixedDeltaTime;
                    m_frame.tick++;
                    onFixedStep(m_frame);
                }
            }

            m_frame.alpha = static_cast<float>(m_accumulator / m_fixedDeltaTime);
            updateRates(frameDeltaTime, m_frame.tick - previousTick);

            return m_frame;
        }

        const FrameContext& frame() const;

        double accumulator() const;

        double framesPerSecond() const;

        double ticksPerSecond() const;

        bool ratesUpdated() const;

    private:
        void updateRates(double elapsed, std::uint64_t ticks);

        double m_fixedDeltaTime;
        double m_maxFrameDeltaTime;
        double m_accumulator = 0.0;

        FrameContext m_frame;
        double m_rateElapsed = 0.0;

        std::uint64_t m_rateFrames = 0;
        std::uint64_t m_rateTicks = 0;

        double m_framesPerSecond = 0.0;
        double m_ticksPerSecond = 0.0;
        bool m_ratesUpdated = false;
    };
}
