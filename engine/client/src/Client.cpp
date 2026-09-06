#include <TechEngine/client/Client.hpp>
#include <TechEngine/platform/window/Window.hpp>

#include <render/RenderThread.hpp>

namespace TechEngine {
    struct Client::State {
        Window window;
        RenderThread renderer;
        bool initialized = false;
    };

    Client::Client() : m_state(std::make_unique<State>()) {
    }

    Client::~Client() {
        stop();
    }

    bool Client::start(int width, int height, std::string_view title) {
        if (m_state->initialized) {
            return false;
        }
        if (!Window::initialize()) {
            return false;
        }
        m_state->initialized = true;
        if (!m_state->window.open(width, height, title) || !m_state->renderer.start(m_state->window)) {
            stop();
            return false;
        }
        return true;
    }

    void Client::pollEvents() {
        if (m_state->initialized) {
            m_state->window.pollEvents();
        }
    }

    bool Client::shouldClose() const {
        if (!m_state->initialized) {
            return true;
        }
        return m_state->window.shouldClose();
    }

    void Client::setTitle(std::string_view title) {
        m_state->window.setTitle(title);
    }

    void Client::stop() {
        m_state->renderer.stop();
        m_state->window.close();
        if (m_state->initialized) {
            Window::terminate();
            m_state->initialized = false;
        }
    }
}
