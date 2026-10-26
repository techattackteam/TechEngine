#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <diagnostics/Diagnostics.hpp>
#include <diagnostics/MemoryTracking.hpp>

#include <exception>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace TechEngine {
    class NoOpTickBarrierServices final : public TickBarrierServices {
    public:
        void assignNetIds(Scene&, std::span<const Entity>) override {
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
                const std::error_code startError = startSimulation();
                if (!startError) {
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
        m_commandQueue.close();
        wakeMainThread();
        m_simulationThread.requestStop();
    }

    std::error_code App::submitCommand(std::unique_ptr<Command> command) {
        const std::error_code submitError = m_commandQueue.submit(std::move(command));
        if (!submitError) {
            m_simulationThread.wakeForCommands();
        }
        return submitError;
    }

    TimingMetrics App::timingMetrics() const {
        TimingMetrics metrics;
        metrics.simulation = m_simulationThread.timing();
        metrics.renderingActive = renderTiming(metrics.render);
        return metrics;
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
        m_resources.closeLoaders();
        m_scene.buildEventStreams(m_eventRegistry);
        m_registry.freeze();
        m_taskGraph = std::make_unique<TaskGraph>(m_schedule, m_eventRegistry);
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

    bool App::renderTiming(RenderTiming&) const {
        return false;
    }

    void App::simulationInit() {
    }

    std::error_code App::simulationStartupLoad(Scene&) {
        return {};
    }

    void App::publishSnapshot(const SimulationContext&) {
    }

    void App::simulationShutdown() {
        m_resources.release(m_sceneHandle);
        m_sceneHandle = {};
        // A released scene holds its references until its entry is freed, so without this drain
        // the cache's shutdown log reports every one of them as a leak.
        m_resources.drainFrees();
    }

    void App::shutdown() {
    }

    std::error_code App::startSimulation() {
        return m_simulationThread.start(m_jobs, *this, renderingActive());
    }

    bool App::renderingActive() const {
        RenderTiming timing;
        return renderTiming(timing);
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

    void App::applyCommands() {
        std::vector<std::unique_ptr<Command>> batch;
        m_commandQueue.consume(batch);
        for (auto& command: batch) {
            command->apply(m_scene);
        }
    }

    void App::cancelCommands() {
        m_commandQueue.close();
        std::vector<std::unique_ptr<Command>> batch;
        m_commandQueue.consume(batch);
        if (batch.empty()) {
            return;
        }
        TE_LOGGER_WARN("Cancelled {0} commands", batch.size());
    }

    bool App::stopSimulation() {
        m_simulationThread.stop();
        const std::error_code startupError = m_simulationThread.startupError();
        if (startupError) {
            TE_LOGGER_ERROR("App simulation startup load failed: {0}", startupError.message());
        }
        const ThreadCompletionResult completion = m_simulationThread.completion();
        if (completion.status != ThreadCompletionStatus::Failed) {
            return !startupError;
        }
        if (completion.failure) {
            reportAppFailure("simulation", completion.failure);
        } else {
            TE_LOGGER_ERROR("App simulation failed without an exception");
        }
        return false;
    }
}
