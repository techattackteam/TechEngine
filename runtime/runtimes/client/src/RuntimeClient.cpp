#include "RuntimeClient.hpp"

#include "core/Logger.hpp"
#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void RuntimeClient::init() {
        m_client.m_systemRegistry.registerSystem<ProjectManager>(std::filesystem::current_path());
        m_client.m_systemRegistry.getSystem<ProjectManager>().init();
        m_client.init(m_client.m_systemRegistry.getSystem<ProjectManager>().getProjectPath());
        m_runFunction = [this]() {
            m_client.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_client.m_systemRegistry.getSystem<ScriptEngine>().loadDLL(
            m_client.m_systemRegistry.getSystem<ProjectManager>().getResourcesPath().string() + "\\client\\scripts\\build\\debug\\ClientScripts.dll");
    }

    void RuntimeClient::start() {
        m_running = true;
        m_client.onStart();
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
        m_client.shutdown();
    }

    Application* createApp() {
        return new RuntimeClient();
    }
}
