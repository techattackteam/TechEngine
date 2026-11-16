#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <render/FramePacketPool.hpp>
#include <render/FrameRenderer.hpp>
#include <render/PacketMailbox.hpp>
#include <render/device/RenderDevice.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace TechEngine {
    class JobSystem;
    class Window;
    class InputBuffer;
    class ImGuiHook;

    class RenderThread {
    private:
        RenderDevice m_device;
        FrameRenderer m_renderer;
        FramePacketPool m_pool;
        PacketMailbox m_mailbox{m_pool};
        mutable std::mutex m_timingMutex;
        RenderTiming m_timing;
        std::atomic<bool> m_vsync = false;
        DedicatedThread m_thread;

    public:
        RenderThread() = default;

        ~RenderThread();

        RenderThread(const RenderThread&) = delete;

        RenderThread& operator=(const RenderThread&) = delete;

        RenderThread(RenderThread&&) = delete;

        RenderThread& operator=(RenderThread&&) = delete;

        bool start(JobSystem& jobs, const Clock& clock, Window& window, const InputBuffer& input, std::function<void()> onFailure = {}, ImGuiHook* imguiHook = nullptr);

        std::unique_ptr<FramePacket> acquirePacket();

        void publish(std::unique_ptr<FramePacket> packet);

        RenderTiming timing() const;

        ThreadCompletionResult completion() const;

        void setVSync(bool enabled);

        void stop();

    private:
        void threadMain(const DedicatedThreadContext& context, const Clock& clock, Window& window, const InputBuffer& input, ImGuiHook* imguiHook);
    };
}
