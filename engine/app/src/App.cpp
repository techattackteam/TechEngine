#include <TechEngine/app/App.hpp>
#include <TechEngine/app/FrameLoop.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/math/Format.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/events/EventStreamManager.hpp>

#include <diagnostics/MemoryTracking.hpp>

#include <chrono>
#include <cstdint>
#include <span>
#include <thread>

namespace TechEngine {
    struct DemoDamage {
        std::uint64_t tick{0};
        std::uint32_t amount{0};
    };

    static constexpr std::uint64_t DEMO_DAMAGE_TICK_PERIOD = 30;

    int run() {
        memoryTrackingAnchor();

        Clock clock;
        FrameLoop loop(Role::Client);

        EventRegistry registry;
        registry.registerEvent<DemoDamage>("TechEngine.DemoDamage");

        EventStreamManager streams{registry};
        EventCursor damageCursor;

        Clock::TimePoint previous = clock.now();
        constexpr std::uint64_t frames = 120;

        constexpr Clock::TimePoint::duration frameBudget = std::chrono::duration_cast<Clock::TimePoint::duration>(std::chrono::duration<double>(FrameLoop::FIXED_DELTA_TIME));

        for (std::uint64_t i = 0; i < frames; ++i) {
            clock.advanceFrame();
            setDiagnosticFrame(clock.frame());

            const Clock::TimePoint current = clock.now();
            const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
            previous = current;

            loop.advance(frameDeltaTime, [&streams](const FrameContext& fixedStep) {
                if (fixedStep.tick % DEMO_DAMAGE_TICK_PERIOD == 0) {
                    streams.publish(DemoDamage{fixedStep.tick, 42});
                }
                streams.makeVisible(fixedStep.frameIndex, fixedStep.tick);
            });

            const FrameContext& frame = loop.frame();
            streams.makeVisible(frame.frameIndex, frame.tick);

            const std::span<const DemoDamage> damage = streams.read<DemoDamage>(damageCursor);
            for (const DemoDamage& event: damage) {
                TE_LOGGER_WARN("DemoDamage {0} from tick {1}, read on frame {2} ({3} in this batch)", event.amount, event.tick, frame.frameIndex, damage.size());
            }

            streams.retire(frame.frameIndex, frame.tick);

            TE_LOGGER_INFO("Frame {0}: deltaTime = {1:.6f}, fixedDeltaTime = {2:.6f}, accumulator = {3:.6f}, tick = {4}, role = {5}", frame.frameIndex, frame.deltaTime, frame.fixedDeltaTime, loop.accumulator(), frame.tick, static_cast<std::uint32_t>(frame.role));

            // TODO(S3): throwaway 60 Hz pacer. Spins because sleep_for rounds up to the
            // ~15.6 ms Windows timer tick; real frame pacing is undecided.
            const Clock::TimePoint frameDeadline = current + frameBudget;
            while (clock.now() < frameDeadline) {
                std::this_thread::yield();
            }

            TE_PROFILER_FRAME();
        }
        Vec3 position = Vec3(1.4f, 3, 4);
        Mat4 matrix = Mat4(1, 2, 3, 4, 5, 67, 8, 9, 1, 2, 3, 54, 6, 1, 2, 2);
        Mat2x3 matrix2x3 = Mat2x3(1, 2, 3, 4, 5, 6);
        TE_LOGGER_INFO("{0:.2f}", position);
        TE_LOGGER_INFO("{0:.2f}", matrix);
        TE_LOGGER_INFO("{0:.2f}", matrix2x3);

        return 0;
    }
}
