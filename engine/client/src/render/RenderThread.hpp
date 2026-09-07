#pragma once

#include <render/FrameCommandBuffer.hpp>
#include <render/FrameRenderer.hpp>

#include <atomic>
#include <future>
#include <stop_token>
#include <thread>

namespace TechEngine {
    class Window;

    class RenderThread {
    private:
        FrameRenderer m_frameRenderer;
        FrameCommandBuffer m_commandBuffer;
        std::atomic<double> m_framesPerSecond = 0.0;
        std::jthread m_thread;

    public:
        RenderThread();

        ~RenderThread();

        RenderThread(const RenderThread&) = delete;

        RenderThread& operator=(const RenderThread&) = delete;

        RenderThread(RenderThread&&) = delete;

        RenderThread& operator=(RenderThread&&) = delete;

        bool start(Window& window);

        void publish(const FrameCommand& command);

        double framesPerSecond() const;

        void stop();

    private:
        void threadMain(const std::stop_token& stopToken, Window& window, std::promise<bool> startup);
    };
}
