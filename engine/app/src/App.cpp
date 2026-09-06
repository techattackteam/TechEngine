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

        Clock::TimePoint previous = m_clock.now();

        while (!shouldClose()) {
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

            TE_PROFILER_FRAME();
        }

        shutdown();

        return 0;
    }
}
