#include <iostream>
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

    void ScriptEngine::init(const std::string &dllPath) {
        if (!dllPath.empty()) {
            m_userCustomDll = LoadLibraryA(dllPath.c_str());
        }
    }

    void ScriptEngine::stop() {
        if (m_userCustomDll) {
            bool result = FreeLibrary(m_userCustomDll);
            if (!result) {
                TE_LOGGER_ERROR("Failed to unload user scripts dll");
            }
            ScriptEngine::getInstance()->deleteScripts();
        }
    }

    void ScriptEngine::onStart() {
        for (Script *script: scripts) {
            script->onStart();
        }
    }

    void ScriptEngine::onUpdate() {
        for (Script *script: scripts) {
            script->onUpdate();
        }
    }

    void ScriptEngine::onFixedUpdate() {
        for (Script *script: scripts) {
            script->onFixedUpdate();
        }
    }

    ScriptEngine *ScriptEngine::getInstance() {
        if (instance == nullptr) {
            return new ScriptEngine();
        } else {
            return instance;
        }
    }

    void ScriptEngine::registerScript(Script *pScript) {
        scripts.push_back(pScript);
    }

    void ScriptEngine::deleteScripts() {
        scripts.clear();
    }
}