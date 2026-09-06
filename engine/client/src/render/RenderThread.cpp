#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <glad/gl.h>

#include <condition_variable>
#include <exception>
#include <mutex>
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
    }

    void RenderThread::threadMain(std::stop_token stopToken, Window& window, std::promise<bool> startup) {
        bool contextClaimed = false;
        bool startupReported = false;
        try {
            window.makeContextCurrent();
            contextClaimed = true;
            const GlProcLoader loader = window.processLoader();
            const int version = loader != nullptr ? gladLoadGL(loader) : 0;
            const bool ready = version != 0 && GLAD_GL_VERSION_4_5 != 0;
            startup.set_value(ready);
            startupReported = true;

            if (ready) {
                TE_LOGGER_INFO("OpenGL {0}; renderer: {1}", reinterpret_cast<const char*>(glGetString(GL_VERSION)), reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
                std::mutex mutex;
                std::condition_variable_any stopped;
                std::unique_lock lock{mutex};
                stopped.wait(lock, std::move(stopToken), [] {
                    return false;
                });
            }
        } catch (...) {
            if (!startupReported) {
                startup.set_exception(std::current_exception());
            }
        }

        if (contextClaimed) {
            window.releaseContext();
        }
    }
}
