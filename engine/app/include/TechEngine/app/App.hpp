#pragma once

#include <TechEngine/app/FrameLoop.hpp>
#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/core/FrameContext.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>

namespace TechEngine {
    class App {
    protected:
        // Declaration order is construction order, and three of these bind to the one above.
        // Reordering compiles cleanly and binds a reference to an unconstructed member.
        MountTable m_mounts;
        FileAccess m_files{m_mounts};
        JobSystem m_jobs;
        EngineContext m_engine{m_files, m_jobs};
        Clock m_clock;
        FrameLoop m_loop;

    public:
        explicit App(Role role);

        virtual ~App() = default;

        // Owns the whole lifecycle: init, the loop, shutdown. Nothing outside calls the
        // hooks below, which is why they are protected.
        int run();

    protected:
        virtual void init() = 0;

        virtual void fixedUpdate(const FrameContext& frame) = 0;

        virtual void update(const FrameContext& frame) = 0;

        virtual void shutdown() = 0;
    };
}
