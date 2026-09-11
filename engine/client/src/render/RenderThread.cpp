#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/time/RateCounter.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>
#include <render/SnapshotHistory.hpp>

#include <glad/gl.h>

#include <chrono>
#include <exception>
#include <utility>

namespace TechEngine {
    RenderThread::~RenderThread() {
        stop();
    }

    bool RenderThread::start(JobSystem& jobs, const Clock& clock, Window& window, const InputBuffer& input, std::function<void()> onFailure) {
        if (m_thread.joinable()) {
            return false;
        }
        m_mailbox.reset();
        {
            const std::lock_guard lock{m_timingMutex};
            m_timing = RenderTiming{};
        }
        try {
            m_thread = jobs.createDedicatedThread("TERender", ThreadRole::Dedicated, [this, &clock, &window, &input, notify = std::move(onFailure)](DedicatedThreadContext& context) {
                try {
                    threadMain(context, clock, window, input);
                } catch (...) {
                    if (notify) {
                        notify();
                    }
                    throw;
                }
            });
            const auto startup = m_thread.waitUntilReady();
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

    void RenderThread::publish(const RenderSnapshot& snapshot) {
        m_mailbox.publish(snapshot);
    }
    void RenderThread::setVSync(bool enabled) {
        m_vsync.store(enabled, std::memory_order_relaxed);
    }
    ThreadCompletionResult RenderThread::completion() const {
        return m_thread.completion();
    }
    RenderTiming RenderThread::timing() const {
        const std::lock_guard lock{m_timingMutex};
        return m_timing;
    }
    void RenderThread::stop() {
        m_thread.requestStop();
        m_thread.join();
        m_mailbox.reset();
    }

    void RenderThread::threadMain(const DedicatedThreadContext& context, const Clock& clock, Window& window, const InputBuffer& input) {
        bool contextClaimed = false;
        std::exception_ptr failure;
        try {
            window.makeContextCurrent();
            contextClaimed = true;
            bool vsync = m_vsync.load(std::memory_order_relaxed);
            window.setVSync(vsync);
            const auto loader = window.processLoader();
            const int version = loader != nullptr ? gladLoadGL(loader) : 0;
            if (version != 0 && GLAD_GL_VERSION_4_5 != 0 && m_renderer.initialize()) {
                SnapshotHistory history;
                RateCounter rate;
                RenderTiming metrics;
                auto previousFrame = clock.now();
                context.signalReady();
                while (!context.stopToken().stop_requested()) {
                    const bool requestedVsync = m_vsync.load(std::memory_order_relaxed);
                    if (requestedVsync != vsync) {
                        window.setVSync(requestedVsync);
                        vsync = requestedVsync;
                    }
                    const auto started = clock.now();
                    RenderSnapshot frame;
                    {
                        TE_PROFILER_SCOPE("Render.PrepareFrame");
                        if (const auto snapshot = m_mailbox.snapshot()) {
                            history.acquire(*snapshot);
                        }
                        frame = history.prepareFrame(started, input.presentationState());
                    }
                    {
                        TE_PROFILER_SCOPE("Render.RenderFrame");
                        m_renderer.draw(frame, window.framebufferSize());
                    }
                    const auto submitted = clock.now();
                    {
                        TE_PROFILER_SCOPE("Render.PresentFrame");
                        window.swapBuffers();
                    }
                    TE_PROFILER_FRAME();
                    const auto completed = clock.now();
                    metrics.frame++;
                    metrics.frameInterval = std::chrono::duration<double>(completed - previousFrame).count();
                    metrics.renderWorkDuration = std::chrono::duration<double>(submitted - started).count();
                    metrics.sampledAt = completed;
                    rate.advance(metrics.frameInterval, 1);
                    metrics.framesPerSecond = rate.rate();
                    previousFrame = completed;
                    const std::lock_guard lock{m_timingMutex};
                    m_timing = metrics;
                }
            }
        } catch (...) {
            failure = std::current_exception();
        }
        if (contextClaimed) {
            m_renderer.shutdown();
            window.releaseContext();
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}
