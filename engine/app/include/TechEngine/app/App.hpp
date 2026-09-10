#pragma once

#include <TechEngine/app/SimulationThread.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/SimulationContext.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>

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
        EngineContext m_engine{m_files, m_jobs};
        SimulationThread m_simulationThread;

    public:
        explicit App(Role role);

        virtual ~App() = default;

        int run();

        void requestStop();

    protected:
        bool stopRequested() const;

        virtual void init() = 0;

        virtual void mainUpdate();

        virtual void simulationInit();

        virtual void fixedUpdate(const SimulationContext& frame);

        virtual void update(const SimulationContext& frame);

        virtual void simulationShutdown();

        virtual void shutdown();

        virtual bool shouldClose() const;
    };
}
