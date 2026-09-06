#pragma once

#include <future>
#include <stop_token>
#include <thread>

namespace TechEngine {
    class Window;

    class RenderThread {
    private:
        std::jthread m_thread;

    public:
        RenderThread();

        ~RenderThread();

        RenderThread(const RenderThread&) = delete;

        RenderThread& operator=(const RenderThread&) = delete;

        RenderThread(RenderThread&&) = delete;

        RenderThread& operator=(RenderThread&&) = delete;

        bool start(Window& window);
        void stop();

    private:
        void threadMain(std::stop_token stopToken, Window& window, std::promise<bool> startup);
    };
}
