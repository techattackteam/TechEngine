#include <TechEngine/app/FrameLoop.hpp>
#include <TechEngine/base/profiler/Profile.hpp>

namespace TechEngine {
    FrameLoop::FrameLoop(Role role, double fixedDeltaTime, double maxFrameDeltaTime) : m_fixedDeltaTime(fixedDeltaTime), m_maxFrameDeltaTime(maxFrameDeltaTime) {
        m_frame.role = role;
        m_frame.fixedDeltaTime = static_cast<float>(fixedDeltaTime);
    }

    const FrameContext& FrameLoop::advance(double frameDeltaTime) {
        TE_PROFILER_FUNCTION();

        if (frameDeltaTime < 0.0) {
            frameDeltaTime = 0.0;
        }

        const double clampedDeltaTime = frameDeltaTime > m_maxFrameDeltaTime ? m_maxFrameDeltaTime : frameDeltaTime;

        m_accumulator += clampedDeltaTime;

        {
            TE_PROFILER_SCOPE("FixedSteps");

            while (m_accumulator >= m_fixedDeltaTime) {
                m_accumulator -= m_fixedDeltaTime;
                m_frame.tick++;
            }
        }

        m_frame.deltaTime = static_cast<float>(clampedDeltaTime);
        m_frame.alpha = static_cast<float>(m_accumulator / m_fixedDeltaTime);
        m_frame.frameIndex++;

        return m_frame;
    }

    const FrameContext& FrameLoop::frame() const {
        return m_frame;
    }

    double FrameLoop::accumulator() const {
        return m_accumulator;
    }
}
