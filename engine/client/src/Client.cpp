#include <TechEngine/client/Client.hpp>
#include <TechEngine/core/EngineContext.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

#include <atomic>
#include <mutex>
#include <utility>

namespace TechEngine {
    struct Client::State {
        Window window;
        RenderThread renderer;
        std::mutex wakeMutex;
        bool initialized = false;
        std::atomic<bool> active = false;
    };

    Client::Client() : m_state(std::make_unique<State>()) {
    }

    Client::~Client() {
        stop();
    }

    bool Client::start(const EngineContext& engine, InputBuffer& input, int width, int height, std::string_view title, std::function<void()> onFailure) {
        {
            const std::lock_guard lock{m_state->wakeMutex};
            if (m_state->initialized || !Window::initialize()) {
                return false;
            }
            m_state->initialized = true;
        }
        try {
            if (!m_state->window.open(width, height, title)) {
                stop();
                return false;
            }
            m_state->window.setInputBuffer(&input);
            if (!m_state->renderer.start(engine.jobs, engine.clock, m_state->window, input, std::move(onFailure))) {
                stop();
                return false;
            }
        } catch (...) {
            stop();
            throw;
        }
        m_state->active.store(true);
        return true;
    }

    void Client::waitEvents() {
        if (m_state->active.load()) {
            m_state->window.waitEvents();
        }
    }

    void Client::waitEvents(double timeoutSeconds) {
        if (m_state->active.load()) {
            m_state->window.waitEvents(timeoutSeconds);
        }
    }

    void Client::wakeMain() {
        const std::lock_guard lock{m_state->wakeMutex};
        if (m_state->initialized) {
            Window::postEmptyEvent();
        }
    }

    void Client::publish(const RenderSnapshot& snapshot) const {
        m_state->renderer.publish(snapshot);
    }

    std::optional<RenderTiming> Client::renderTiming() const {
        if (!m_state->active.load()) {
            return std::nullopt;
        }
        return m_state->renderer.timing();
    }

    void Client::setTitle(std::string_view title) const {
        m_state->window.setTitle(title);
    }

    void Client::setVSync(bool enabled) const {
        m_state->renderer.setVSync(enabled);
    }

    bool Client::shouldClose() const {
        return !m_state->active.load() || m_state->window.shouldClose();
    }

    bool Client::failed() const {
        return m_state->renderer.completion().status == ThreadCompletionStatus::Failed;
    }

    void Client::stop() {
        m_state->active.store(false);
        m_state->renderer.stop();
        const std::lock_guard lock{m_state->wakeMutex};
        m_state->window.setInputBuffer(nullptr);
        m_state->window.close();
        if (m_state->initialized) {
            Window::terminate();
            m_state->initialized = false;
        }
    }
}
