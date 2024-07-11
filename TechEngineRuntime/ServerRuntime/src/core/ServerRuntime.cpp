#include "ServerRuntime.hpp"

#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    ServerRuntime::ServerRuntime() : Server() {
    }

    void ServerRuntime::init() {
        project.loadProject(std::filesystem::current_path().string(), ProjectType::Server);
        Server::init();
        systemsRegistry.getSystem<SceneManager>().loadScene(project.lastLoadedScene);
#ifdef TE_DEBUG
        systemsRegistry.getSystem<ScriptEngine>().init(project.getUserScriptsDLLPath().string(), &systemsRegistry.getSystem<EventDispatcher>());
        TE_LOGGER_INFO("Debug");
#elif TE_RELEASEDEBUG
        systemsRegistry.getSystem<ScriptEngine>().init(project.getUserScriptsDLLPath().string(), &systemsRegistry.getSystem<EventDispatcher>());
        TE_LOGGER_INFO("ReleaseDebug");
#elif TE_RELEASE
        systemsRegistry.getSystem<ScriptEngine>().init(project.getUserScriptsDLLPath().string(), &systemsRegistry.getSystem<EventDispatcher>());
        TE_LOGGER_INFO("Release");
#endif
        systemsRegistry.getSystem<ScriptEngine>().onStart();
        systemsRegistry.getSystem<PhysicsEngine>().start();
    }

    void ServerRuntime::onUpdate() {
        Server::onUpdate();
    }

    void ServerRuntime::onFixedUpdate() {
        Server::onFixedUpdate();
        systemsRegistry.getSystem<PhysicsEngine>().onFixedUpdate();
    }
}
