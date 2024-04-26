#include "ServerRuntime.hpp"

#include "external/EntryPoint.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    ServerRuntime::ServerRuntime() : Server() {
        instance = this;
        timer.init();
        projectManager.loadRuntimeProject(std::filesystem::current_path().string());
#ifdef TE_DEBUG
        ScriptEngine::getInstance()->init(projectManager.getServerScriptsDebugDLLPath().string());
#elif TE_RELEASEDEBUG
        ScriptEngine::getInstance()->init(projectManager.getServerScriptsReleaseDLLPath().string());
#elif TE_RELEASE
        ScriptEngine::getInstance()->init(projectManager.getServerScriptsReleaseDLLPath().string());
#endif
        ScriptEngine::getInstance()->onStart();
        physicsEngine.start();
        init();
    }

    ServerRuntime::~ServerRuntime() {
    }


    void ServerRuntime::onUpdate() {
    }

    void ServerRuntime::onFixedUpdate() {
    }
}

TechEngine::AppCore* TechEngine::createApp() {
    return new TechEngine::ServerRuntime();
}
