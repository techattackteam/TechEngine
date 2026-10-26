#pragma once

#include <TechEngine/app/Command.hpp>
#include <TechEngine/app/CommandQueue.hpp>
#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/events/EventRegistry.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/systems/Schedule.hpp>
#include <TechEngine/core/systems/SerialExecutor.hpp>
#include <TechEngine/core/systems/TaskGraph.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <system_error>

namespace TechEngineTests {
    class AppTestAccess;
}

namespace TechEngine {
    struct SceneBytes;

    class App {
        friend class SimulationThread;
        friend class TechEngineTests::AppTestAccess;

    private:
        std::atomic<bool> m_stopRequested = false;
        std::mutex m_mainMutex;
        std::condition_variable m_mainWake;
        CommandQueue m_commandQueue;

    protected:
        MountTable m_mounts;
        FileAccess m_files{m_mounts};
        JobSystem m_jobs;
        Clock m_clock;
        InputBuffer m_input{m_clock};
        ComponentRegistry m_registry;
        EventRegistry m_eventRegistry;

    private:
        Scene m_scene;

    protected:
        Schedule m_schedule;
        ResourceCache m_resources{m_jobs};
        ResourceHandle<SceneBytes> m_sceneHandle;
        EngineContext m_engine{m_files, m_jobs, m_clock};

    private:
        std::unique_ptr<TaskGraph> m_taskGraph;
        std::unique_ptr<SerialExecutor> m_serialExecutor;
        bool m_simulationFinalized = false;

    protected:
        SimulationThread m_simulationThread;

    public:
        explicit App(Role role);

        virtual ~App() = default;

        int run();

        void requestStop();

        std::error_code submitCommand(std::unique_ptr<Command> command);

        TimingMetrics timingMetrics() const;

    protected:
        bool stopRequested() const;

        virtual void init() = 0;

        virtual void configureSimulation();

        void finalizeSimulation();

        void executeSimulationTick(const SimulationContext& simulation);

        virtual void mainThreadUpdate();

        // Runs on whichever thread requested the stop, so an override must be thread-safe.
        virtual void wakeMainThread();

        virtual bool shouldClose() const;

        // Read from any thread through timingMetrics(), so an override must be thread-safe.
        virtual bool renderTiming(RenderTiming& out) const;

        virtual void simulationInit();

        virtual std::error_code simulationStartupLoad(Scene& scene);

        virtual void publishSnapshot(const SimulationContext& simulation);

        // An override must call this one, or the scene's entry is still counted at cache shutdown.
        virtual void simulationShutdown();

        virtual void shutdown();

    private:
        std::error_code startSimulation();

        bool renderingActive() const;

        void runMainThread();

        void applyCommands();

        void cancelCommands();

        bool stopSimulation();
    };
}
