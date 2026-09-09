#pragma once

#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <render/FrameCommandBuffer.hpp>
#include <render/FrameRenderer.hpp>

#include <atomic>

namespace TechEngine {
    class JobSystem;
    class Window;

    class RenderThread {
    private:
        FrameRenderer m_frameRenderer;
        FrameCommandBuffer m_commandBuffer;
        std::atomic<double> m_framesPerSecond = 0.0;
        DedicatedThread m_thread;

    public:
        RenderThread();

        ~RenderThread();

        RenderThread(const RenderThread&) = delete;

        RenderThread& operator=(const RenderThread&) = delete;

        RenderThread(RenderThread&&) = delete;

        RenderThread& operator=(RenderThread&&) = delete;

        bool start(JobSystem& jobs, Window& window);

        void publish(const FrameCommand& command);

        double framesPerSecond() const;

        void stop();

    private:
        void threadMain(DedicatedThreadContext& context, Window& window);
    };
}
