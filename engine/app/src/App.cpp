#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <diagnostics/Diagnostics.hpp>
#include <diagnostics/MemoryTracking.hpp>

#include <cstdint>
#include <exception>
#include <span>
#include <string_view>

namespace TechEngine {
    class NoOpTickBarrierServices final : public TickBarrierServices {
    public:
        void assignNetIds(Scene&, std::span<const Entity>) override {
        }

        void flushEvents(std::uint64_t, std::uint64_t) override {
        }
    };

    static void reportAppFailure(std::string_view stage, const std::exception_ptr& failure) {
        try {
            std::rethrow_exception(failure);
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("App {0} failed: {1}", stage, error.what());
        } catch (...) {
            TE_LOGGER_ERROR("App {0} failed with a non-std exception", stage);
        }
    }

    App::App(Role role) : m_scene(m_registry), m_schedule(m_registry), m_simulationThread(m_engine, role, SimulationSettings{.input = &m_input, .diagnosticClock = &m_clock}) {
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
                finalizeSimulation();
            }
            if (!stopRequested()) {
                simulationAttempted = true;
                if (startSimulation()) {
                    runMainThread();
                }
            }
        } catch (...) {
            reportAppFailure("startup or main update", std::current_exception());
            result = 1;
        }

        requestStop();
        if (simulationAttempted && !stopSimulation()) {
            result = 1;
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
        wakeMainThread();
        m_simulationThread.requestStop();
    }

    TimingMetrics App::timingMetrics() const {
        return TimingMetrics{m_simulationThread.timing(), renderTiming()};
    }

    bool App::stopRequested() const {
        return m_stopRequested.load();
    }

    void App::configureSimulation() {
    }

    void App::finalizeSimulation() {
        if (m_simulationFinalized) {
            return;
        }
        if (m_registry.find(componentTypeId<Hierarchy>()) == nullptr) {
            m_registry.registerComponent<Hierarchy>(Hierarchy::tag);
        }
        if (m_registry.find(componentTypeId<Transform>()) == nullptr) {
            m_registry.registerComponent<Transform>(Transform::tag);
        }
        configureSimulation();
        m_registry.freeze();
        m_taskGraph = std::make_unique<TaskGraph>(m_schedule);
        m_serialExecutor = std::make_unique<SerialExecutor>(*m_taskGraph);
        m_simulationFinalized = true;
    }

    void App::executeSimulationTick(const SimulationContext& simulation) {
        TE_CHECK(m_serialExecutor != nullptr, "Simulation executor must be finalized before ticking");
        NoOpTickBarrierServices barrier;
        m_serialExecutor->execute(m_scene, simulation, barrier);
    }

    void App::mainThreadUpdate() {
        TE_PROFILER_SCOPE("Main.Wait");
        std::unique_lock lock{m_mainMutex};
        m_mainWake.wait(lock, [this] {
            return stopRequested();
        });
    }

    void App::wakeMainThread() {
    }

    bool App::shouldClose() const {
        return false;
    }

    std::optional<RenderTiming> App::renderTiming() const {
        return std::nullopt;
    }

    void App::simulationInit() {
    }

    void App::publishSnapshot(const SimulationContext&) {
    }

    void App::simulationShutdown() {
    }

    void App::shutdown() {
    }

    bool App::startSimulation() {
        return m_simulationThread.start(m_jobs, *this, renderTiming().has_value());
    }

    void App::runMainThread() {
        while (!stopRequested()) {
            if (shouldClose()) {
                requestStop();
                return;
            }
            mainThreadUpdate();
        }
    }

    bool App::stopSimulation() {
        m_simulationThread.stop();
        const ThreadCompletionResult completion = m_simulationThread.completion();
        if (completion.status != ThreadCompletionStatus::Failed) {
            return true;
        }
        if (completion.failure) {
            reportAppFailure("simulation", completion.failure);
        } else {
            TE_LOGGER_ERROR("App simulation failed without an exception");
        }
        return false;
    }
}
