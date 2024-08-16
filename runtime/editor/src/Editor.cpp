#include "Editor.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    Editor::Editor() : Application(), m_entry(m_systemRegistry) {
    }

    void Editor::init() {
        m_systemRegistry.registerSystem<Timer>();
        m_systemRegistry.registerSystem<Logger>("TechEngineEditor");

        m_systemRegistry.getSystem<Timer>().init();
        m_systemRegistry.getSystem<Logger>().init();
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
        m_systemRegistry.getSystem<Timer>().onStart();
        m_client.onStart();
        m_server.onStart();
        TE_LOGGER_INFO("Editor started");
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
