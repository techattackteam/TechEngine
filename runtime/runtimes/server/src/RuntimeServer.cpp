#include "RuntimeServer.hpp"

#include "project/ProjectManager.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void RuntimeServer::init() {
        m_server.m_systemRegistry.registerSystem<ProjectManager>(std::filesystem::current_path());
        m_server.m_systemRegistry.getSystem<ProjectManager>().init();
        m_server.init(m_server.m_systemRegistry.getSystem<ProjectManager>().getProjectPath(), m_server.m_systemRegistry.getSystem<ProjectManager>().getProjectConfigs());

        m_runFunction = [this]() {
            m_server.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_server.m_systemRegistry.getSystem<ScriptEngine>().loadDLL(
            m_server.m_systemRegistry.getSystem<ProjectManager>().getResourcesPath().string() + "\\server\\scripts\\build\\debug\\ServerScripts.dll");
    }

    void RuntimeServer::start() {
        m_server.onStart();
        m_running = true;
    }

    void RuntimeServer::update() {
        m_server.onUpdate();
    }

    void RuntimeServer::fixedUpdate() {
        m_server.onFixedUpdate();
    }

    void RuntimeServer::stop() {
        m_server.onStop();
    }

    void RuntimeServer::shutdown() {
        m_server.shutdown();
    }

    Application* createApp() {
        return new RuntimeServer();
    }
}
