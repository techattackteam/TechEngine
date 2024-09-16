#include "Server.hpp"

#include "core/Logger.hpp"
#include "core/Timer.hpp"

namespace TechEngine {
    Server::Server() : Core(AppType::Server), m_entry(m_systemRegistry) {
    }

    void Server::init(const std::filesystem::path& rootPath, std::unordered_map<ProjectConfig, std::string>& projectConfigs) {
        m_systemRegistry.registerSystem<Logger>("TechEngineServer");
        m_systemRegistry.getSystem<Logger>().init();
        Core::init(rootPath, projectConfigs);
    }

    void Server::onStart() {
        Core::onStart();
    }

    void Server::onFixedUpdate() {
        Core::onFixedUpdate();
    }

    void Server::onUpdate() {
        Core::onUpdate();
    }

    void Server::onStop() {
        Core::onStop();
    }

    void Server::shutdown() {
        Core::shutdown();
    }
}
