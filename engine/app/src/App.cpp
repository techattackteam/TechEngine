#include <TechEngine/app/App.hpp>

#include "TechEngine/base/diagnostics/Log.hpp"
#include "diagnostics/Diagnostics.hpp"
#include "diagnostics/MemoryTracking.hpp"

namespace TechEngine {

    App::App(Role role) : m_loop(m_engine, role) {
        memoryTrackingAnchor();

        TE_LOGGER_INFO("App constructed with role: {0}", toString(role));
    }

    int App::run() {
        const DiagnosticsScope diagnostics;

        init();

        // The baseline is taken after init(), so a slow init does not land on frame 0 as one
        // huge delta that the loop then clamps and spends its catch-up ticks on.
        Clock::TimePoint previous = m_clock.now();
        constexpr std::uint64_t frames = 120;

        constexpr Clock::TimePoint::duration frameBudget = std::chrono::duration_cast<Clock::TimePoint::duration>(std::chrono::duration<double>(FrameLoop::FIXED_DELTA_TIME));

        for (std::uint64_t i = 0; i < frames; ++i) {
            m_clock.advanceFrame();
            setDiagnosticFrame(m_clock.frame());

            const Clock::TimePoint current = m_clock.now();
            const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
            previous = current;

            const FrameContext& frame = m_loop.frame();

            m_loop.advance(frameDeltaTime, [this](const FrameContext& fixedStep) {
                fixedUpdate(fixedStep);
            });

            update(frame);

            const Clock::TimePoint frameDeadline = current + frameBudget;
            while (m_clock.now() < frameDeadline) {
                std::this_thread::yield();
            }

            TE_PROFILER_FRAME();
        }

        shutdown();

        return 0;
    }
}
