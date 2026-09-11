#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>

#include <diagnostics/Diagnostics.hpp>
#include <diagnostics/MemoryTracking.hpp>

#include <exception>
#include <string_view>

namespace TechEngine {
    static void reportAppFailure(std::string_view stage, const std::exception_ptr& failure) {
        try {
            std::rethrow_exception(failure);
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("App {0} failed: {1}", stage, error.what());
        } catch (...) {
            TE_LOGGER_ERROR("App {0} failed with a non-std exception", stage);
        }
    }

    App::App(Role role) : m_simulationThread(m_engine, role, SimulationSettings{.input = &m_input, .diagnosticClock = &m_clock}) {
        memoryTrackingAnchor();
    }

    int App::run() {
        const DiagnosticsScope diagnostics;
        const ThreadRegistration main = m_jobs.registerCurrentThread("TEMain", ThreadRole::Main);
        m_stopRequested.store(false);
        bool initialized = false;
        bool simulationAttempted = false;
        int result = 0;

        try {
            init();
            initialized = true;
            if (!stopRequested()) {
                simulationAttempted = true;
                if (m_simulationThread.start(m_jobs, *this, renderTiming().has_value())) {
                    while (!stopRequested()) {
                        if (shouldClose()) {
                            requestStop();
                            break;
                        }
                        mainUpdate();
                    }
                }
            }
        } catch (...) {
            reportAppFailure("startup or main update", std::current_exception());
            result = 1;
        }

        requestStop();
        if (simulationAttempted) {
            m_simulationThread.stop();
            const ThreadCompletionResult completion = m_simulationThread.completion();
            if (completion.status == ThreadCompletionStatus::Failed) {
                if (completion.failure) {
                    reportAppFailure("simulation", completion.failure);
                } else {
                    TE_LOGGER_ERROR("App simulation failed without an exception");
                }
                result = 1;
            }
        }

        if (initialized) {
            try {
                shutdown();
            } catch (...) {
                reportAppFailure("shutdown", std::current_exception());
                result = 1;
            }
        }
        return result;
    }

    void App::requestStop() {
        {
            const std::lock_guard lock{m_mainMutex};
            m_stopRequested.store(true);
        }
        m_mainWake.notify_all();
        wakeMain();
        m_simulationThread.requestStop();
    }

    TimingMetrics App::timingMetrics() const {
        return TimingMetrics{m_simulationThread.timing(), renderTiming()};
    }

    bool App::stopRequested() const {
        return m_stopRequested.load();
    }

    void App::mainUpdate() {
        TE_PROFILER_SCOPE("Main.Wait");
        std::unique_lock lock{m_mainMutex};
        m_mainWake.wait(lock, [this] {
            return stopRequested();
        });
    }

    void App::wakeMain() {
    }

    bool App::shouldClose() const {
        return false;
    }

    std::optional<RenderTiming> App::renderTiming() const {
        return std::nullopt;
    }

    void App::simulationInit() {
    }

    void App::fixedUpdate(const SimulationContext&) {
    }

    void App::publishSnapshot(const SimulationContext&) {
    }

    void App::simulationShutdown() {
    }

    void App::shutdown() {
    }
}
