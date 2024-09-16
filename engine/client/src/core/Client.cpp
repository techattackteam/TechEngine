#include "Client.hpp"

#include "core/Logger.hpp"
#include "project/ProjectManager.hpp"
#include "renderer/Renderer.hpp"

namespace TechEngine {
    Client::Client() : Core(AppType::Client), m_entry(m_systemRegistry) {
    }

    void Client::init(const std::filesystem::path& rootPath, std::unordered_map<ProjectConfig, std::string>& projectConfigs) {
        m_systemRegistry.registerSystem<Logger>("TechEngineClient");
        m_systemRegistry.getSystem<Logger>().init();
        Core::init(rootPath, projectConfigs);
        m_systemRegistry.registerSystem<Renderer>(m_systemRegistry);
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
