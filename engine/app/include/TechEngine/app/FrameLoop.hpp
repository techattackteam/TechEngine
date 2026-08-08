#pragma once

#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/FrameContext.hpp>

#include <concepts>

namespace TechEngine {
    class FrameLoop {
    public:
        static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;
        static constexpr double MAX_FRAME_DELTA_TIME = 0.25;

        explicit FrameLoop(Role role, double fixedDeltaTime = FIXED_DELTA_TIME, double maxFrameDeltaTime = MAX_FRAME_DELTA_TIME);

        const FrameContext& advance(double frameDeltaTime);

        template<std::invocable<const FrameContext&> Step>
        const FrameContext& advance(double frameDeltaTime, Step&& onFixedStep) {
            TE_PROFILER_FUNCTION();

            if (frameDeltaTime < 0.0) {
                frameDeltaTime = 0.0;
            }

            const double clampedDeltaTime = frameDeltaTime > m_maxFrameDeltaTime ? m_maxFrameDeltaTime : frameDeltaTime;

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

            return m_frame;
        }

        const FrameContext& frame() const;

        double accumulator() const;

    private:
        double m_fixedDeltaTime;
        double m_maxFrameDeltaTime;
        double m_accumulator = 0.0;
        FrameContext m_frame{};
    };
}
