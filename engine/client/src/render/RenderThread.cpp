#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <glad/gl.h>

#include <chrono>
#include <exception>

namespace TechEngine {
    RenderThread::RenderThread() = default;

    RenderThread::~RenderThread() {
        stop();
    }

    bool RenderThread::start(JobSystem& jobs, Window& window) {
        if (m_thread.joinable()) {
            return false;
        }

        m_framesPerSecond.store(0.0, std::memory_order_relaxed);
        try {
            m_thread = jobs.createDedicatedThread("TERender", ThreadRole::Dedicated, [this, &window](DedicatedThreadContext& context) {
                threadMain(context, window);
            });
            const ThreadStartupResult startup = m_thread.waitUntilReady();
            if (startup.status == ThreadStartupStatus::Ready) {
                return true;
            }
            if (startup.failure) {
                std::rethrow_exception(startup.failure);
            }
        } catch (const std::exception& error) {
            TE_LOGGER_ERROR("Render thread startup failed: {0}", error.what());
        } catch (...) {
            TE_LOGGER_ERROR("Render thread startup failed with a non-std exception");
        }

        stop();
        return false;
    }

    void RenderThread::stop() {
        if (m_thread.joinable()) {
            m_thread.requestStop();
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

    void RenderThread::threadMain(DedicatedThreadContext& context, Window& window) {
        const std::stop_token stopToken = context.stopToken();
        bool contextClaimed = false;
        std::exception_ptr failure;
        try {
            window.makeContextCurrent();
            contextClaimed = true;
            window.setVSync(true);
            const GlProcLoader loader = window.processLoader();
            const int version = loader != nullptr ? gladLoadGL(loader) : 0;
            m_commandBuffer.reset();
            const bool ready = version != 0 && GLAD_GL_VERSION_4_5 != 0 && m_frameRenderer.initialize();

            if (ready) {
                context.signalReady();
                using RateClock = std::chrono::steady_clock;
                RateClock::time_point rateStarted = RateClock::now();
                std::uint64_t renderedFrames = 0;
                while (!stopToken.stop_requested()) {
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
            failure = std::current_exception();
        }

        if (contextClaimed) {
            m_frameRenderer.shutdown();
            window.releaseContext();
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}
