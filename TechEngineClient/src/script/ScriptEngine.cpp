#include "ScriptEngine.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    ScriptEngine::ScriptEngine() {
        if (instance != nullptr) {
            delete this;
        } else {
            instance = this;
        }
    }

    void ScriptEngine::init(const std::string& dllPath) {
        if (std::filesystem::exists(dllPath)) {
            m_userCustomDll = LoadLibraryA(dllPath.c_str());
        } else {
            TE_LOGGER_ERROR("Failed to load user scripts dll");
        }
    }

    void ScriptEngine::stop() {
        if (m_userCustomDll) {
            deleteScripts();
            bool result = FreeLibrary(m_userCustomDll);
            if (!result) {
                TE_LOGGER_ERROR("Failed to unload user scripts dll");
            }
        }
    }

    void ScriptEngine::onStart() {
        for (Script* script: scripts) {
            script->onStart();
        }
    }

    void ScriptEngine::onUpdate() {
        for (Script* script: scripts) {
            script->onUpdate();
        }
    }

    void ScriptEngine::onFixedUpdate() {
        for (Script* script: scripts) {
            script->onFixedUpdate();
        }
    }

    ScriptEngine* ScriptEngine::getInstance() {
        if (instance == nullptr) {
            TE_LOGGER_CRITICAL("ScriptEngine instance is nullptr in getInstance() method.");
        }
        return instance;
    }

    void ScriptEngine::registerScript(Script* script) {
        getInstance()->scripts.push_back(script);
    }

    void ScriptEngine::deleteScripts() {
        for (Script* script: scripts) {
            delete script;
        }
        scripts.clear();
    }
}
