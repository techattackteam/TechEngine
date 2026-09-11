#pragma once

#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/base/time/RateCounter.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <atomic>
#include <concepts>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace TechEngine {
    class App;
    class JobSystem;

    struct SimulationSettings {
        static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;
        static constexpr double MAX_ELAPSED_TIME = 0.25;

        double fixedDeltaTime = FIXED_DELTA_TIME;
        double maxElapsedTime = MAX_ELAPSED_TIME;
        InputBuffer* input = nullptr;
        Clock* diagnosticClock = nullptr;
    };

    class SimulationThread {
    private:
        double m_fixedDeltaTime;
        double m_maxElapsedTime;
        InputBuffer* m_input;
        Clock* m_diagnosticClock;
        bool m_presentationActive = false;
        double m_accumulator = 0.0;
        Clock::TimePoint m_origin{};
        Clock::TimePoint m_tickStarted{};
        double m_tickWorkDuration = 0.0;
        RateCounter m_rate;
        InputFrame m_inputFrame;
        SimulationContext m_context;

        mutable std::mutex m_timingMutex;
        SimulationTiming m_timing;

        std::atomic<bool> m_stopRequested = false;
        std::mutex m_waitMutex;
        std::condition_variable m_wake;
        DedicatedThread m_thread;

    public:
        SimulationThread(const EngineContext& engine, Role role, const SimulationSettings& settings = {});

        ~SimulationThread();

        SimulationThread(const SimulationThread&) = delete;

        SimulationThread& operator=(const SimulationThread&) = delete;

        SimulationThread(SimulationThread&&) = delete;

        SimulationThread& operator=(SimulationThread&&) = delete;

        bool start(JobSystem& jobs, App& app, bool presentationActive);

        void requestStop();

        void stop();

        ThreadCompletionResult completion() const;

        SimulationTiming timing() const;

        // Everything below is owner-only: a synchronous caller must keep the thread stopped.
        void restartTimeline(Clock::TimePoint origin);

        const SimulationContext& advance(double elapsed);

        template<std::invocable<const SimulationContext&> Step>
        const SimulationContext& advance(double elapsed, Step&& onFixedStep) {
            TE_PROFILER_FUNCTION();
            const std::uint64_t previousTick = m_context.tick;
            const double measured = beginAdvance(elapsed);
            while (m_accumulator >= m_fixedDeltaTime) {
                TE_PROFILER_SCOPE("Simulation.Tick");
                beginTick();
                onFixedStep(m_context);
                endTick();
            }
            endAdvance(measured, previousTick);
            return m_context;
        }

        const SimulationContext& simulationContext() const;

        double accumulator() const;

    private:
        double beginAdvance(double elapsed);

        void beginTick();

        void endTick();

        void endAdvance(double elapsed, std::uint64_t previousTick);

        void waitForNextTick(Clock::TimePoint sampled);

        void threadMain(const DedicatedThreadContext& context, App& app);
    };
}
