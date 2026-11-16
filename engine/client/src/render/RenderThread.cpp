#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/base/time/RateCounter.hpp>
#include <TechEngine/client/render/ImGuiHook.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/input/InputBuffer.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include "GlRequirements.hpp"
#include <render/FrameView.hpp>
#include <render/PacketHistory.hpp>
#include <render/RenderThread.hpp>

#include <glad/gl.h>

#include <chrono>
#include <exception>
#include <utility>

namespace TechEngine {
    RenderThread::~RenderThread() {
        stop();
    }

    bool RenderThread::start(JobSystem& jobs, const Clock& clock, Window& window, const InputBuffer& input, std::function<void()> onFailure, ImGuiHook* imguiHook) {
        if (m_thread.joinable()) {
            return false;
        }
        m_mailbox.reset();
        {
            const std::lock_guard lock{m_timingMutex};
            m_timing = RenderTiming{};
        }
        try {
            m_thread = jobs.createDedicatedThread("TERender", ThreadRole::Dedicated, [this, &clock, &window, &input, imguiHook, notify = std::move(onFailure)](DedicatedThreadContext& context) {
                try {
                    threadMain(context, clock, window, input, imguiHook);
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

    std::unique_ptr<FramePacket> RenderThread::acquirePacket() {
        return m_pool.acquire();
    }

    void RenderThread::publish(std::unique_ptr<FramePacket> packet) {
        m_mailbox.publish(std::move(packet));
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

    void RenderThread::threadMain(const DedicatedThreadContext& context, const Clock& clock, Window& window, const InputBuffer& input, ImGuiHook* imguiHook) {
        bool contextClaimed = false;
        bool imguiStarted = false;
        std::exception_ptr failure;
        try {
            window.makeContextCurrent();
            contextClaimed = true;
            bool vsync = m_vsync.load(std::memory_order_relaxed);
            window.setVSync(vsync);
            const auto loader = window.processLoader();
            const int version = loader != nullptr ? gladLoadGL(loader) : 0;
            const GlSupport glSupport{
                .loadedVersion = version,
                .core46 = GLAD_GL_VERSION_4_6 != 0,
                .bindlessTextures = GLAD_GL_ARB_bindless_texture != 0,
            };
            if (meetsGlRequirements(glSupport) && m_renderer.initialize(m_device)) {
                PacketHistory history{m_pool};
                FrameView frame;
                RateCounter rate;
                RenderTiming metrics;
                auto previousFrame = clock.now();
                if (imguiHook != nullptr) {
                    imguiHook->start();
                    imguiStarted = true;
                }
                context.signalReady();

                while (!context.stopToken().stop_requested()) {
                    const bool requestedVsync = m_vsync.load(std::memory_order_relaxed);
                    if (requestedVsync != vsync) {
                        window.setVSync(requestedVsync);
                        vsync = requestedVsync;
                    }
                    const auto started = clock.now();
                    {
                        TE_PROFILER_SCOPE("Render.PrepareFrame");
                        m_mailbox.deliverTo(history);
                        history.prepareFrame(started, input.presentationState(), frame);
                    }
                    {
                        TE_PROFILER_SCOPE("Render.RenderFrame");
                        const FramebufferSize size = window.framebufferSize();
                        m_renderer.draw(frame, RenderTargetHandle{}, RenderExtent{size.width, size.height});
                    }
                    if (imguiStarted) {
                        imguiHook->draw();
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
            if (imguiStarted) {
                imguiHook->stop();
            }
            m_renderer.shutdown();
            m_device.shutdown();
            window.releaseContext();
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
    }
}
