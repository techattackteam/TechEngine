#pragma once

#include <TechEngine/base/time/Clock.hpp>
#include <TechEngine/core/TimingMetrics.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <render/FramePacketPool.hpp>
#include <render/FrameRenderer.hpp>
#include <render/PacketMailbox.hpp>
#include <render/UploadChannel.hpp>
#include <render/ViewSlot.hpp>
#include <render/device/RenderDevice.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace TechEngine {
    class JobSystem;
    class Window;
    class InputBuffer;
    class ImGuiHook;
    class GpuCopies;
    class ViewTargets;
    struct FrameView;

    class RenderThread {
    private:
        RenderDevice m_device;
        FrameRenderer m_renderer;
        FramePacketPool m_pool;
        PacketMailbox m_mailbox{m_pool};
        UploadChannel m_uploads;
        ViewSlot m_views;
        // Render thread only.
        bool m_viewTargetsFailed = false;
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

        void sendUploads(std::vector<ResourceUploadRecord>& records);

        void takeFinishedUploads(std::vector<FinishedUpload>& out);

        void setViews(std::span<const ViewRequest> views);

        RenderTiming timing() const;

        ThreadCompletionResult completion() const;

        void setVSync(bool enabled);

        void stop();

    private:
        void threadMain(const DedicatedThreadContext& context, const Clock& clock, Window& window, const InputBuffer& input, ImGuiHook* imguiHook);

        void applyUploads(GpuCopies& copies, std::vector<ResourceUploadRecord>& records, std::vector<FinishedUpload>& finished);

        void drawViews(const FrameView& frame, const GpuCopies& copies, std::span<const ViewRequest> requests, ViewTargets& targets, const RenderExtent& windowExtent);
    };
}
