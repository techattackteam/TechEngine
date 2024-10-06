#include "Client.hpp"

#include "core/Logger.hpp"
#include "project/Project.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    Client::Client() : m_entry(m_systemRegistry) {
    }

    void Client::registerSystems(const std::filesystem::path& rootPath) {
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();
        Core::registerSystems(rootPath);
        m_systemRegistry.registerSystem<Renderer>(m_systemRegistry);
    }

    void Client::init() {
        Core::init(AppType::Client);
        m_systemRegistry.getSystem<Renderer>().init();
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
