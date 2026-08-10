#include <TechEngine/app/FrameLoop.hpp>

namespace TechEngine {
    FrameLoop::FrameLoop(const Role role, const double fixedDeltaTime, const double maxFrameDeltaTime) : m_fixedDeltaTime(fixedDeltaTime), m_maxFrameDeltaTime(maxFrameDeltaTime) {
        m_frame.role = role;
        m_frame.fixedDeltaTime = static_cast<float>(fixedDeltaTime);
    }

    const FrameContext& FrameLoop::advance(double frameDeltaTime) {
        return advance(frameDeltaTime, [](const FrameContext&) {
        });
    }

    const FrameContext& FrameLoop::frame() const {
        return m_frame;
    }

    double FrameLoop::accumulator() const {
        return m_accumulator;
    }
}
