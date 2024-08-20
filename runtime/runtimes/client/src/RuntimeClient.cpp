#include "RuntimeClient.hpp"

#include "core/Logger.hpp"
#include "project/ProjectManager.hpp"

namespace TechEngine {
    void RuntimeClient::init() {
        m_client.init();
        m_client.m_systemRegistry.registerSystem<ProjectManager>(std::filesystem::current_path());
        m_client.m_systemRegistry.getSystem<ProjectManager>().init();
        m_runFunction = [this]() {
            m_client.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
    }

    void RuntimeClient::start() {
        m_client.onStart();
        m_running = true;
    }

    void RuntimeClient::fixedUpdate() {
        m_client.onFixedUpdate();
    }

    void RuntimeClient::update() {
        m_client.onUpdate();
    }

    void RuntimeClient::stop() {
        m_client.onStop();
    }

    void RuntimeClient::shutdown() {
        m_client.destroy();
    }

    Application* createApp() {
        return new RuntimeClient();
    }
}
