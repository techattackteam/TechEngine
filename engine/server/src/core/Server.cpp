#include "Server.hpp"

#include "core/Logger.hpp"
#include "project/Project.hpp"

namespace TechEngine {
    Server::Server() : m_entry(m_systemRegistry) {
    }

    void Server::registerSystems(const std::filesystem::path& rootPath) {
        m_systemRegistry.registerSystem<Logger>("TechEngineServer");
        m_systemRegistry.getSystem<Logger>().init();
        Core::registerSystems(rootPath);
    }

    void Server::init() {
        Core::init(AppType::Server);
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
