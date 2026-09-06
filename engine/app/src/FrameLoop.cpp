#include <TechEngine/app/FrameLoop.hpp>

namespace TechEngine {
    FrameLoop::FrameLoop(const EngineContext& engine, const Role role, const double fixedDeltaTime, const double maxFrameDeltaTime) : m_fixedDeltaTime(fixedDeltaTime), m_maxFrameDeltaTime(maxFrameDeltaTime), m_frame{.engine = engine} {
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

    double FrameLoop::framesPerSecond() const {
        return m_framesPerSecond;
    }

    double FrameLoop::ticksPerSecond() const {
        return m_ticksPerSecond;
    }

    bool FrameLoop::ratesUpdated() const {
        return m_ratesUpdated;
    }

    void FrameLoop::updateRates(double elapsed, std::uint64_t ticks) {
        m_ratesUpdated = false;
        m_rateElapsed += elapsed;
        m_rateFrames++;
        m_rateTicks += ticks;
        if (m_rateElapsed >= 1.0) {
            m_framesPerSecond = static_cast<double>(m_rateFrames) / m_rateElapsed;
            m_ticksPerSecond = static_cast<double>(m_rateTicks) / m_rateElapsed;
            m_rateElapsed = 0.0;
            m_rateFrames = 0;
            m_rateTicks = 0;
            m_ratesUpdated = true;
        }
    }
}
