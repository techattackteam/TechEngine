#include <TechEngine/app/App.hpp>
#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/time/Clock.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>

namespace TechEngine {
    SimulationThread::SimulationThread(const EngineContext& engine, const Role role, const double fixedDeltaTime, const double maxFrameDeltaTime) : m_fixedDeltaTime(fixedDeltaTime), m_maxFrameDeltaTime(maxFrameDeltaTime), m_simulationContext{.engine = engine} {
        m_simulationContext.role = role;
        m_simulationContext.fixedDeltaTime = static_cast<float>(fixedDeltaTime);
    }

    SimulationThread::~SimulationThread() {
        stop();
    }

    bool SimulationThread::start(JobSystem& jobSystem, App& app) {
        if (m_thread.joinable()) {
            return false;
        }

        m_ticksPerSecond.store(0.0, std::memory_order_relaxed);
        m_thread = jobSystem.createDedicatedThread("TESimulation", ThreadRole::Dedicated, [this, &app](DedicatedThreadContext& context) {
            threadMain(context, app);
        });
        if (m_thread.waitUntilReady().status == ThreadStartupStatus::Ready) {
            return true;
        }

        stop();
        return false;
    }

    void SimulationThread::requestStop() {
        m_thread.requestStop();
    }

    void SimulationThread::stop() {
        m_thread.requestStop();
        m_thread.join();
    }

    ThreadCompletionResult SimulationThread::completion() const {
        return m_thread.completion();
    }

    const SimulationContext& SimulationThread::advance(double frameDeltaTime) {
        return advance(frameDeltaTime, [](const SimulationContext&) {
        });
    }

    const SimulationContext& SimulationThread::simulationContext() const {
        return m_simulationContext;
    }

    double SimulationThread::accumulator() const {
        return m_accumulator;
    }
    double SimulationThread::timeUntilNextTick() const {
        return m_fixedDeltaTime - m_accumulator;
    }
    std::uint64_t SimulationThread::tick() const {
        return m_publishedTick.load(std::memory_order_relaxed);
    }

    double SimulationThread::ticksPerSecond() const {
        return m_ticksPerSecond;
    }
    std::uint64_t SimulationThread::rateSampleIndex() const {
        return m_rateSampleIndex;
    }

    void SimulationThread::updateRates(double elapsed, std::uint64_t ticks) {
        m_rateElapsed += elapsed;
        m_rateTicks += ticks;

        if (m_rateElapsed >= 1.0) {
            m_ticksPerSecond = static_cast<double>(m_rateTicks) / m_rateElapsed;
            m_rateSampleIndex++;

            m_rateElapsed = 0.0;
            m_rateTicks = 0;
        }
    }

    void SimulationThread::threadMain(const DedicatedThreadContext& context, App& app) {
        app.simulationInit();

        std::exception_ptr failure;
        try {
            const std::stop_token stopToken = context.stopToken();
            Clock clock;
            std::mutex waitMutex;
            std::condition_variable_any wake;
            context.signalReady();
            Clock::TimePoint previous = clock.now();
            while (!stopToken.stop_requested()) {
                clock.advanceFrame();
                setDiagnosticFrame(clock.frame());
                const Clock::TimePoint current = clock.now();
                const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
                previous = current;
                const std::uint64_t previousTick = m_simulationContext.tick;
                const SimulationContext& newContext = advance(frameDeltaTime, [&app](const SimulationContext& fixedStep) {
                    app.fixedUpdate(fixedStep);
                });
                if (newContext.tick > previousTick) {
                    app.update(newContext);
                }

                TE_PROFILER_FRAME();

                const Clock::TimePoint nextTick = current + std::chrono::ceil<Clock::TimePoint::duration>(std::chrono::duration<double>(timeUntilNextTick()));
                std::unique_lock lock{waitMutex};
                wake.wait_until(lock, stopToken, nextTick, [] {
                    return false;
                });
            }
        } catch (...) {
            failure = std::current_exception();
        }

        try {
            app.simulationShutdown();
        } catch (...) {
            if (!failure) {
                throw;
            }
            TE_LOGGER_ERROR("Simulation shutdown also failed; preserving the original simulation failure");
        }

        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}
