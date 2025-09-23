#include "Client.hpp"

#include "audio/AudioSystem.hpp"
#include "core/Logger.hpp"
#include "input/Input.hpp"
#include "project/Project.hpp"
#include "renderer/Renderer.hpp"
#include "ui/WidgetsRegistry.hpp"
#include "window/Viewport.hpp"

namespace TechEngine {
    Client::Client() : m_entry(m_systemRegistry) {
    }

    void Client::registerSystems(const std::filesystem::path& rootPath) {
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();
        Core::registerSystems(rootPath);
        m_systemRegistry.registerSystem<AudioSystem>(m_systemRegistry);
        m_systemRegistry.registerSystem<Renderer>(m_systemRegistry);
        m_systemRegistry.registerSystem<WidgetsRegistry>(m_systemRegistry);
        m_systemRegistry.registerSystem<Viewport>();
        m_systemRegistry.registerSystem<Input>(m_systemRegistry);
    }

    void Client::init() {
        Core::init(AppType::Client);
        m_systemRegistry.getSystem<Renderer>().init();
        m_systemRegistry.getSystem<AudioSystem>().init();
        m_systemRegistry.getSystem<WidgetsRegistry>().init();
        m_systemRegistry.getSystem<Viewport>().init();
        m_systemRegistry.getSystem<Input>().init();
    }

    void Client::onStart() {
        Core::onStart();
    }

    void Client::onFixedUpdate() {
        Core::onFixedUpdate();
    }

    void Client::onUpdate() {
        Core::onUpdate();
    }

    void Client::onStop() {
        Core::onStop();
    }

    void Client::shutdown() {
        Core::shutdown();
    }
}
