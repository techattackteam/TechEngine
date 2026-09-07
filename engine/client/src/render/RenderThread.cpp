#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <glad/gl.h>

#include <chrono>
#include <cstdint>
#include <exception>
#include <utility>

namespace TechEngine {
    RenderThread::RenderThread() = default;

    RenderThread::~RenderThread() {
        stop();
    }

    bool RenderThread::start(Window& window) {
        if (m_thread.joinable()) {
            return false;
        }

        m_framesPerSecond.store(0.0, std::memory_order_relaxed);
        std::promise<bool> startup;
        std::future<bool> ready = startup.get_future();
        try {
            m_thread = std::jthread([this, &window, startup = std::move(startup)](std::stop_token stopToken) mutable {
                threadMain(std::move(stopToken), window, std::move(startup));
            });
            if (ready.get()) {
                return true;
            }
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("Render thread startup failed: {0}", error.what());
        }

        stop();
        return false;
    }

    void RenderThread::stop() {
        if (m_thread.joinable()) {
            m_thread.request_stop();
            m_thread.join();
        }
        m_commandBuffer.reset();
        m_framesPerSecond.store(0.0, std::memory_order_relaxed);
    }

    void RenderThread::publish(const FrameCommand& command) {
        m_commandBuffer.publish(command);
    }

    double RenderThread::framesPerSecond() const {
        return m_framesPerSecond.load(std::memory_order_relaxed);
    }

    void RenderThread::threadMain(const std::stop_token& stopToken, Window& window, std::promise<bool> startup) {
        TE_PROFILER_THREAD_NAME("TERender");
        bool contextClaimed = false;
        bool startupReported = false;
        try {
            window.makeContextCurrent();
            window.setVSync(true);
            contextClaimed = true;
            const GlProcLoader loader = window.processLoader();
            const int version = loader != nullptr ? gladLoadGL(loader) : 0;
            m_commandBuffer.reset();
            const bool ready = version != 0 && GLAD_GL_VERSION_4_5 != 0 && m_frameRenderer.initialize();

            startup.set_value(ready);
            startupReported = true;

            if (ready) {
                using RateClock = std::chrono::steady_clock;
                RateClock::time_point rateStarted = RateClock::now();
                std::uint64_t renderedFrames = 0;
                while (!stopToken.stop_requested() && !window.shouldClose()) {
                    {
                        TE_PROFILER_SCOPE("RenderThread.Present");
                        const FrameCommand command = m_commandBuffer.snapshot();
                        const FramebufferSize size = window.framebufferSize();
                        m_frameRenderer.draw(command, size);
                        window.swapBuffers();
                    }
                    renderedFrames++;
                    const RateClock::time_point now = RateClock::now();
                    const double elapsed = std::chrono::duration<double>(now - rateStarted).count();
                    if (elapsed >= 1.0) {
                        m_framesPerSecond.store(static_cast<double>(renderedFrames) / elapsed, std::memory_order_relaxed);
                        renderedFrames = 0;
                        rateStarted = now;
                    }
                }
            }
        } catch (...) {
            if (!startupReported) {
                startup.set_exception(std::current_exception());
            }
        }

        if (contextClaimed) {
            m_frameRenderer.shutdown();
            window.releaseContext();
        }
    }
}
