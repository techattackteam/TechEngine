#include "RuntimeServer.hpp"

#include "project/Project.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    void RuntimeServer::registerSystems() {
        m_server.m_systemRegistry.registerSystem<Project>(std::filesystem::current_path());
    }

    void RuntimeServer::init() {
        m_server.m_systemRegistry.getSystem<Project>().init();
        m_server.init();

        m_runFunction = [this]() {
            m_server.m_entry.run([this]() {
                                     fixedUpdate();
                                 }, [this]() {
                                     update();
                                 });
        };
        m_server.m_systemRegistry.getSystem<ScriptEngine>().loadDLL(
            m_server.m_systemRegistry.getSystem<Project>().getPath(PathType::Resources, AppType::Server).string() + "\\server\\scripts\\build\\debug\\ServerScripts.dll");
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
