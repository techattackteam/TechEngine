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

        const Clock::TimePoint::duration frameBudget = std::chrono::duration_cast<Clock::TimePoint::duration>(std::chrono::duration<double>(FrameLoop::FIXED_DELTA_TIME));

        for (std::uint64_t i = 0; i < frames; ++i) {
            clock.advanceFrame();
            setDiagnosticFrame(clock.frame());

            const Clock::TimePoint current = clock.now();
            const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
            previous = current;

            loop.advance(frameDeltaTime);

            TE_LOGGER_INFO(
                "Frame {0}: deltaTime = {1:.6f}, fixedDeltaTime = {2:.6f}, accumulator = {3:.6f}, tick = {4}, role = {5}",
                loop.frame().frameIndex,
                loop.frame().deltaTime,
                loop.frame().fixedDeltaTime,
                loop.accumulator(),
                loop.frame().tick,
                static_cast<std::uint32_t>(loop.frame().role));

            // TODO(S3): throwaway 60 Hz pacer. Spins because sleep_for rounds up to the
            // ~15.6 ms Windows timer tick; real frame pacing is undecided.
            const Clock::TimePoint frameDeadline = current + frameBudget;
            while (clock.now() < frameDeadline) {
                std::this_thread::yield();
            }
        }

        return 0;
    }
}
