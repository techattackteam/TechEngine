#include <TechEngine/app/App.hpp>
#include <TechEngine/app/FrameLoop.hpp>
#include <TechEngine/base/Clock.hpp>
#include <TechEngine/base/Log.hpp>

#include <chrono>
#include <cstdint>
#include <thread>

namespace TechEngine {
    int run() {
        Clock clock;
        FrameLoop loop(Role::Client);

        Clock::TimePoint previous = clock.now();
        constexpr std::uint64_t frames = 120;

        for (std::uint64_t i = 0; i < frames; ++i) {
            clock.advanceFrame();
            setDiagnosticFrame(clock.frame());

            const Clock::TimePoint current = clock.now();
            const double frameDt = std::chrono::duration<double>(current - previous).count();
            previous = current;

            loop.advance(frameDt);

            TE_LOGGER_INFO("Frame {0}: dt = {1:.6f}, fixedDt = {2:.6f}, accumulator = {3:.6f}, tick = {4}, role = {5}",
                loop.frame().frameIndex,
                loop.frame().dt,
                loop.frame().fixedDt,
                loop.accumulator(),
                loop.frame().tick,
                static_cast<std::uint32_t>(loop.frame().role)
            );

            std::this_thread::sleep_for(std::chrono::duration<double>(FrameLoop::FIXED_DT));
        }

        return 0;
    }
}
