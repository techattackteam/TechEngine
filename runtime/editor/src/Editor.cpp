#include "Editor.hpp"

#include "core/Timer.hpp"

namespace TechEngine {
    Editor::Editor() : Application(), m_entry(m_systemRegistry) {
    }

    void Editor::init() {
        m_systemRegistry.registerSystem<Timer>();
        m_runFunction = [this]() {
            m_entry.run([this]() {
                            fixedUpdate();
                        }, [this]() {
                            update();
                        });
        };
        m_client.init();
        m_server.init();
        m_running = true;
    }

    void Editor::start() {
    }

    void Editor::update() {
    }

    void Editor::fixedUpdate() {
    }

    void Editor::stop() {
    }

    void Editor::destroy() {
        m_client.destroy();
        m_server.destroy();
    }

    Application* createApp() {
        return new Editor();
    }
}
