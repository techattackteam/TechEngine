#include <TechEngine/app/App.hpp>
#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>

#include <chrono>
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
        try {

            m_ticksPerSecond.store(0.0, std::memory_order_relaxed);
            m_thread = jobSystem.createDedicatedThread("TESimulation", ThreadRole::Dedicated, [this, &app](DedicatedThreadContext& context) {
                threadMain(context, app);
            });
            const ThreadStartupResult startup = m_thread.waitUntilReady();
            if (startup.status == ThreadStartupStatus::Ready) {
                return true;
            }
            if (startup.failure) {
                std::rethrow_exception(startup.failure);
            }
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("Simulation thread startup failed: {0}", error.what());
        } catch (...) {
            TE_LOGGER_ERROR("Simulation thread startup failed with a non-std exception");
        }

        stop();
        return false;
    }

    void SimulationThread::requestStop() {
        m_thread.requestStop();
        m_wake.notify_all();
    }

    void SimulationThread::stop() {
        if (m_thread.joinable()) {
            m_thread.requestStop();
            m_wake.notify_all();
            m_thread.join();
        }
    }

    ThreadCompletionResult SimulationThread::completion() const {
        return m_thread.completion();
    }

    const SimulationContext& SimulationThread::advance(double frameDeltaTime) {
        return advance(frameDeltaTime, [](const SimulationContext&) {
        });
    }

    const SimulationContext& SimulationThread::step() const {
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

    bool SimulationThread::ratesUpdated() const {
        return m_ratesUpdated;
    }

    void SimulationThread::updateRates(double elapsed, std::uint64_t ticks) {
        m_ratesUpdated = false;
        m_rateElapsed += elapsed;
        m_rateTicks += ticks;

        if (m_rateElapsed >= 1.0) {
            m_ticksPerSecond = static_cast<double>(m_rateTicks) / m_rateElapsed;
            m_rateSampleIndex++;

            m_rateElapsed = 0.0;
            m_rateTicks = 0;

            m_ratesUpdated = true;
        }
    }

    void SimulationThread::threadMain(const DedicatedThreadContext& context, App& app) {
        const std::stop_token stopToken = context.stopToken();
        bool initialized = false;
        std::exception_ptr failure;
        try {
            app.simulationInit();
            initialized = true;
            context.signalReady();
            Clock::TimePoint previous = m_clock.now();
            while (!stopToken.stop_requested()) {
                m_clock.advanceFrame();
                setDiagnosticFrame(m_clock.frame());
                const Clock::TimePoint current = m_clock.now();
                const double frameDeltaTime = std::chrono::duration<double>(current - previous).count();
                previous = current;
                const std::uint64_t previousTick = m_simulationContext.tick;
                const SimulationContext& newContex = advance(frameDeltaTime, [&app](const SimulationContext& fixedStep) {
                    app.fixedUpdate(fixedStep);
                });
                if (newContex.tick > previousTick) {
                    app.update(newContex);
                }

                TE_PROFILER_FRAME();

                const Clock::TimePoint nextTick = current + std::chrono::ceil<Clock::TimePoint::duration>(std::chrono::duration<double>(timeUntilNextTick()));
                std::unique_lock lock{m_waitMutex};
                m_wake.wait_until(lock, stopToken, nextTick, [] {
                    return false;
                });
            }
        } catch (...) {
            failure = std::current_exception();
        }

        if (initialized) {
            try {
                app.simulationShutdown();
            } catch (...) {
                if (!failure) {
                    failure = std::current_exception();
                } else {
                    TE_LOGGER_ERROR("Simulation shutdown also failed; preserving the original simulation failure");
                }
            }
        }

        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}
