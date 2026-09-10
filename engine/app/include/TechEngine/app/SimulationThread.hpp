#pragma once

#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <atomic>
#include <concepts>
#include <cstdint>

namespace TechEngine {
    class App;
    class JobSystem;

    class SimulationThread {
    private:
        double m_fixedDeltaTime;
        double m_maxFrameDeltaTime;
        double m_accumulator = 0.0;

        SimulationContext m_simulationContext;
        double m_rateElapsed = 0.0;

        std::uint64_t m_rateTicks = 0;

        std::atomic<std::uint64_t> m_publishedTick = 0;
        std::atomic<double> m_ticksPerSecond = 0.0;
        std::atomic<std::uint64_t> m_rateSampleIndex = 0;

        DedicatedThread m_thread;

    public:
        static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;
        static constexpr double MAX_FRAME_DELTA_TIME = 0.25;

        SimulationThread(const EngineContext& engine, Role role, double fixedDeltaTime = FIXED_DELTA_TIME, double maxFrameDeltaTime = MAX_FRAME_DELTA_TIME);

        ~SimulationThread();

        SimulationThread(const SimulationThread&) = delete;

        SimulationThread& operator=(const SimulationThread&) = delete;

        SimulationThread(SimulationThread&&) = delete;

        SimulationThread& operator=(SimulationThread&&) = delete;

        bool start(JobSystem& jobs, App& app);

        void requestStop();

        void stop();

        ThreadCompletionResult completion() const;

        // External callers may advance synchronously only while the dedicated thread is stopped.
        const SimulationContext& advance(double frameDeltaTime);

        template<std::invocable<const SimulationContext&> Step>
        const SimulationContext& advance(double frameDeltaTime, Step&& onFixedStep) {
            TE_PROFILER_FUNCTION();

            if (frameDeltaTime < 0.0) {
                frameDeltaTime = 0.0;
            }

            const double clampedDeltaTime = frameDeltaTime > m_maxFrameDeltaTime ? m_maxFrameDeltaTime : frameDeltaTime;
            const std::uint64_t previousTick = m_simulationContext.tick;

            m_simulationContext.iterationIndex++;
            m_simulationContext.deltaTime = static_cast<float>(clampedDeltaTime);

            m_accumulator += clampedDeltaTime;

            {
                TE_PROFILER_SCOPE("FixedSteps");

                while (m_accumulator >= m_fixedDeltaTime) {
                    m_accumulator -= m_fixedDeltaTime;
                    m_simulationContext.tick++;
                    onFixedStep(m_simulationContext);
                }
            }

            m_simulationContext.alpha = static_cast<float>(m_accumulator / m_fixedDeltaTime);
            updateRates(frameDeltaTime, m_simulationContext.tick - previousTick);
            m_publishedTick.store(m_simulationContext.tick, std::memory_order_relaxed);
            return m_simulationContext;
        }

        // The live context and accumulator may only be read by the thread advancing the loop.
        const SimulationContext& simulationContext() const;

        double accumulator() const;

        std::uint64_t tick() const;

        double ticksPerSecond() const;

        std::uint64_t rateSampleIndex() const;

    private:
        void threadMain(const DedicatedThreadContext& context, App& app);

        double timeUntilNextTick() const;

        void updateRates(double elapsed, std::uint64_t ticks);
    };
}
