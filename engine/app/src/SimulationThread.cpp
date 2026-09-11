#include <TechEngine/app/App.hpp>
#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <chrono>
#include <cmath>
#include <exception>

namespace TechEngine {
    static Clock::TimePoint::duration toClockDuration(double seconds) {
        return std::chrono::duration_cast<Clock::TimePoint::duration>(std::chrono::duration<double>(seconds));
    }

    static double toSeconds(Clock::TimePoint::duration duration) {
        return std::chrono::duration<double>(duration).count();
    }

    SimulationThread::SimulationThread(const EngineContext& engine, const Role role, const SimulationSettings& settings)
        : m_fixedDeltaTime(settings.fixedDeltaTime), m_maxElapsedTime(settings.maxElapsedTime), m_input(settings.input), m_diagnosticClock(settings.diagnosticClock), m_context{.fixedDeltaTime = settings.fixedDeltaTime, .role = role, .input = m_inputFrame, .engine = engine} {
        TE_CHECK(std::isfinite(m_fixedDeltaTime) && m_fixedDeltaTime > 0.0, "Fixed delta time must be finite and positive");
        TE_CHECK(std::isfinite(m_maxElapsedTime) && m_maxElapsedTime > 0.0, "Maximum elapsed time must be finite and positive");
        if (m_input != nullptr) {
            m_inputFrame.events.reserve(m_input->capacity());
        }
        restartTimeline(engine.clock.now());
    }

    SimulationThread::~SimulationThread() {
        stop();
    }

    bool SimulationThread::start(JobSystem& jobs, App& app, const bool presentationActive) {
        if (m_thread.joinable()) {
            return false;
        }
        m_stopRequested.store(false);
        m_presentationActive = presentationActive;
        m_thread = jobs.createDedicatedThread("TESimulation", ThreadRole::Dedicated, [this, &app](DedicatedThreadContext& context) {
            try {
                threadMain(context, app);
            } catch (...) {
                app.requestStop();
                throw;
            }
            app.requestStop();
        });
        if (m_thread.waitUntilReady().status == ThreadStartupStatus::Ready) {
            return true;
        }
        stop();
        return false;
    }

    void SimulationThread::requestStop() {
        {
            const std::lock_guard lock{m_waitMutex};
            m_stopRequested.store(true);
        }
        m_wake.notify_all();
    }

    void SimulationThread::stop() {
        requestStop();
        m_thread.requestStop();
        m_thread.join();
    }

    ThreadCompletionResult SimulationThread::completion() const {
        return m_thread.completion();
    }

    SimulationTiming SimulationThread::timing() const {
        const std::lock_guard lock{m_timingMutex};
        return m_timing;
    }

    void SimulationThread::restartTimeline(const Clock::TimePoint origin) {
        m_accumulator = 0.0;
        m_tickWorkDuration = 0.0;
        m_rate = RateCounter{};
        m_origin = origin;
        m_context.tick = 0;
        m_context.tickTime = origin;
        m_context.timeline++;
        m_inputFrame.events.clear();
        m_inputFrame.held = InputState{};
        m_inputFrame.recovered = false;
        const std::lock_guard lock{m_timingMutex};
        m_timing = SimulationTiming{.timeline = m_context.timeline, .sampledAt = origin};
    }

    const SimulationContext& SimulationThread::advance(const double elapsed) {
        return advance(elapsed, [](const SimulationContext&) {
        });
    }

    const SimulationContext& SimulationThread::simulationContext() const {
        return m_context;
    }

    double SimulationThread::accumulator() const {
        return m_accumulator;
    }

    double SimulationThread::beginAdvance(double elapsed) {
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            elapsed = 0.0;
        }
        if (elapsed > m_maxElapsedTime) {
            m_origin += toClockDuration(elapsed - m_maxElapsedTime);
            m_context.timeline++;
            m_accumulator += m_maxElapsedTime;
        } else {
            m_accumulator += elapsed;
        }
        return elapsed;
    }

    void SimulationThread::beginTick() {
        m_accumulator -= m_fixedDeltaTime;
        m_context.tick++;
        m_context.tickTime = m_origin + toClockDuration(static_cast<double>(m_context.tick) * m_fixedDeltaTime);
        if (m_input != nullptr) {
            m_input->consume(m_inputFrame);
            if (m_inputFrame.recovered) {
                TE_LOGGER_WARN("Input overflow: recovered held state after losing sequences {0} through {1}", m_inputFrame.firstLostSequence, m_inputFrame.lastLostSequence);
            }
        }
        m_tickStarted = m_context.engine.clock.now();
    }

    void SimulationThread::endTick() {
        m_tickWorkDuration = toSeconds(m_context.engine.clock.now() - m_tickStarted);
        if (m_diagnosticClock == nullptr) {
            return;
        }
        m_diagnosticClock->advanceFrame();
        setDiagnosticFrame(m_diagnosticClock->frame());
        if (m_presentationActive) {
            TE_PROFILER_FRAME_NAMED("SimulationTicks");
        } else {
            TE_PROFILER_FRAME();
        }
    }

    void SimulationThread::endAdvance(const double elapsed, const std::uint64_t previousTick) {
        m_rate.advance(elapsed, m_context.tick - previousTick);
        const SimulationTiming sample{.tick = m_context.tick, .timeline = m_context.timeline, .ticksPerSecond = m_rate.rate(), .tickWorkDuration = m_tickWorkDuration, .sampledAt = m_context.engine.clock.now()};
        const std::lock_guard lock{m_timingMutex};
        m_timing = sample;
    }

    void SimulationThread::waitForNextTick(const Clock::TimePoint sampled) {
        TE_PROFILER_SCOPE("Simulation.Wait");
        const Clock::TimePoint deadline = sampled + std::chrono::ceil<Clock::TimePoint::duration>(std::chrono::duration<double>(m_fixedDeltaTime - m_accumulator));
        std::unique_lock lock{m_waitMutex};
        m_wake.wait_until(lock, deadline, [this] {
            return m_stopRequested.load();
        });
    }

    void SimulationThread::threadMain(const DedicatedThreadContext& context, App& app) {
        app.simulationInit();
        std::exception_ptr failure;
        try {
            restartTimeline(m_context.engine.clock.now());
            {
                TE_PROFILER_SCOPE("Simulation.Publish");
                app.publishSnapshot(m_context);
            }
            context.signalReady();

            const std::stop_token stopToken = context.stopToken();
            Clock::TimePoint previous = m_origin;
            while (!m_stopRequested.load() && !app.stopRequested() && !stopToken.stop_requested()) {
                const Clock::TimePoint current = m_context.engine.clock.now();
                const std::uint64_t previousTick = m_context.tick;
                advance(toSeconds(current - previous), [&app](const SimulationContext& simulation) {
                    app.fixedUpdate(simulation);
                });
                previous = current;
                if (m_context.tick != previousTick) {
                    TE_PROFILER_SCOPE("Simulation.Publish");
                    app.publishSnapshot(m_context);
                }
                waitForNextTick(current);
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
