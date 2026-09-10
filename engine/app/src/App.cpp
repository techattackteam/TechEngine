#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>

#include <diagnostics/Diagnostics.hpp>
#include <diagnostics/MemoryTracking.hpp>

#include <chrono>
#include <exception>
#include <string_view>

namespace TechEngine {

    static constexpr auto MAIN_COMPLETION_POLL_INTERVAL = std::chrono::milliseconds{10};

    static void reportAppFailure(std::string_view stage, const std::exception_ptr& failure) {
        try {
            std::rethrow_exception(failure);
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("App {0} failed: {1}", stage, error.what());
        } catch (...) {
            TE_LOGGER_ERROR("App {0} failed with a non-std exception", stage);
        }
    }

    App::App(Role role) : m_simulationThread(m_engine, role) {
        memoryTrackingAnchor();

        TE_LOGGER_INFO("App constructed with role: {0}", toString(role));
    }

    int App::run() {
        const DiagnosticsScope diagnostics;
        const ThreadRegistration main = m_jobs.registerCurrentThread("TEMain", ThreadRole::Main);

        m_stopRequested.store(false);
        bool mainInitialized = false;
        bool simulationAttempted = false;
        bool canMainPump = true;
        int result = 0;

        try {
            init();
            mainInitialized = true;

            if (!m_stopRequested.load()) {
                simulationAttempted = true;
                if (!m_simulationThread.start(m_jobs, *this)) {
                    TE_LOGGER_ERROR("App simulation startup failed");
                    result = 1;
                } else {
                    while (!m_stopRequested.load() && m_simulationThread.completion().status == ThreadCompletionStatus::Running) {
                        if (shouldClose()) {
                            requestStop();
                            break;
                        }
                        mainUpdate();
                        TE_PROFILER_FRAME();
                    }
                }
            }
        } catch (...) {
            reportAppFailure("startup or main update", std::current_exception());
            canMainPump = false;
            result = 1;
        }

        requestStop();
        if (simulationAttempted) {
            m_simulationThread.requestStop();
            while (m_simulationThread.completion().status == ThreadCompletionStatus::Running) {
                if (canMainPump) {
                    try {
                        mainUpdate();
                    } catch (...) {
                        reportAppFailure("main update during shutdown", std::current_exception());
                        canMainPump = false;
                        result = 1;
                    }
                }

                std::unique_lock lock{m_mainMutex};
                m_mainWake.wait_for(lock, MAIN_COMPLETION_POLL_INTERVAL, [this] {
                    return m_simulationThread.completion().status != ThreadCompletionStatus::Running;
                });
            }

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

        if (mainInitialized) {
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
            std::lock_guard const lock{m_mainMutex};
            m_stopRequested.store(true);
        }
        m_mainWake.notify_all();
    }

    void App::mainUpdate() {
        std::unique_lock lock{m_mainMutex};
        m_mainWake.wait_for(lock, MAIN_COMPLETION_POLL_INTERVAL, [this] {
            return m_stopRequested.load() || m_simulationThread.completion().status != ThreadCompletionStatus::Running;
        });
    }
    void App::simulationInit() {
    }

    void App::simulationShutdown() {
    }

    void App::fixedUpdate(const SimulationContext&) {
    }

    void App::update(const SimulationContext&) {
    }

    void App::shutdown() {
    }

    bool App::shouldClose() const {
        return false;
    }
}
