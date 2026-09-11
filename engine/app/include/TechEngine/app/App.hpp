#pragma once

#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>

namespace TechEngine {
    class App {
        friend class SimulationThread;

    private:
        std::atomic<bool> m_stopRequested = false;
        std::mutex m_mainMutex;
        std::condition_variable m_mainWake;

    protected:
        MountTable m_mounts;
        FileAccess m_files{m_mounts};
        JobSystem m_jobs;
        Clock m_clock;
        EngineContext m_engine{m_files, m_jobs, m_clock};
        InputBuffer m_input{m_clock};
        SimulationThread m_simulationThread;

    public:
        explicit App(Role role);

        virtual ~App() = default;

        int run();

        void requestStop();

        TimingMetrics timingMetrics() const;

    protected:
        bool stopRequested() const;

        virtual void init() = 0;

        virtual void mainUpdate();

        // Runs on whichever thread requested the stop, so an override must be thread-safe.
        virtual void wakeMain();

        virtual bool shouldClose() const;

        // Read from any thread through timingMetrics(), so an override must be thread-safe.
        virtual std::optional<RenderTiming> renderTiming() const;

        virtual void simulationInit();

        virtual void fixedUpdate(const SimulationContext& simulation);

        virtual void publishSnapshot(const SimulationContext& simulation);

        virtual void simulationShutdown();

        virtual void shutdown();
    };
}
