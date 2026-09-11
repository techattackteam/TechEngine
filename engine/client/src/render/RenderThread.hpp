#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <render/FrameRenderer.hpp>
#include <render/SnapshotMailbox.hpp>

#include <atomic>
#include <functional>
#include <mutex>

namespace TechEngine {
    class JobSystem;
    class Window;
    class InputBuffer;

    class RenderThread {
    private:
        FrameRenderer m_renderer;
        SnapshotMailbox m_mailbox;
        mutable std::mutex m_timingMutex;
        RenderTiming m_timing;
        std::atomic<bool> m_vsync = true;
        DedicatedThread m_thread;

    public:
        RenderThread() = default;

        ~RenderThread();

        RenderThread(const RenderThread&) = delete;

        RenderThread& operator=(const RenderThread&) = delete;

        RenderThread(RenderThread&&) = delete;

        RenderThread& operator=(RenderThread&&) = delete;

        bool start(JobSystem& jobs, const Clock& clock, Window& window, const InputBuffer& input, std::function<void()> onFailure = {});

        void publish(const RenderSnapshot& snapshot);

        RenderTiming timing() const;

        ThreadCompletionResult completion() const;

        void setVSync(bool enabled);

        void stop();

    private:
        void threadMain(const DedicatedThreadContext& context, const Clock& clock, Window& window, const InputBuffer& input);
    };
}
