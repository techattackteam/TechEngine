#include <TechEngine/app/FrameLoop.hpp>

namespace TechEngine {
    FrameLoop::FrameLoop(Role role, double fixedDt, double maxFrameDt) : m_fixedDt(fixedDt), m_maxFrameDt(maxFrameDt) {
        m_frame.role = role;
        m_frame.fixedDt = static_cast<float>(fixedDt);
    }

    const FrameContext& FrameLoop::advance(double frameDt) {
        const double clamped = frameDt > m_maxFrameDt ? m_maxFrameDt : frameDt;

        m_accumulator += clamped;

        while (m_accumulator >= m_fixedDt) {
            m_accumulator -= m_fixedDt;
            m_frame.tick++;
        }

        m_frame.dt = static_cast<float>(clamped);
        m_frame.alpha = static_cast<float>(m_accumulator / m_fixedDt);
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
