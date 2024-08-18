#include "core/Logger.hpp"
#include "ScriptEngine.hpp"
#include "script/ScriptCrashHandler.hpp"
#include <filesystem>

namespace TechEngine {
    ScriptEngine::ScriptEngine(SystemsRegistry& systemsRegistry) : m_systemRegistry(systemsRegistry) {
    }

    void ScriptEngine::init() {
        scriptRegister = ScriptRegister::getInstance();
        scriptRegister->init(this);
    }

    void ScriptEngine::loadDLL(const std::string& dllPath) {
        if (std::filesystem::exists(dllPath)) {
            loadingScripts = true;
            m_userCustomDll = LoadLibraryA(dllPath.c_str());
            if (!m_userCustomDll) {
                TE_LOGGER_ERROR("Failed to load user scripts dll {0}", GetLastError());
                return;
            }
            if (m_APIEntryPoint == nullptr) {
                TE_LOGGER_ERROR("API entry point not set. Cannot load user scripts dll.");
                return;
            }
            m_APIEntryPoint(&m_systemRegistry);
            loadingScripts = false;
            dllLoaded = true;
        } else {
            TE_LOGGER_WARN("User scripts dll not found. Skipping loading.");
        }
    }

    void ScriptEngine::shutdown() {
        stop();
        delete scriptRegister;
    }

    void ScriptEngine::stop() {
        if (m_userCustomDll) {
            deleteScripts();
            bool result = FreeLibrary(m_userCustomDll);
            if (!result) {
                TE_LOGGER_ERROR("Failed to unload user scripts dll");
            }
            dllLoaded = false;
        }
    }

    void ScriptEngine::onStart() {
        if (dllLoaded) {
            for (Script* script: scripts) {
                //RUN_SCRIPT_FUNCTION(script, onStart);
                script->onStartFunc();
            }
        }
    }

    void ScriptEngine::onUpdate() {
        if (dllLoaded) {
            for (Script* script: scripts) {
                //RUN_SCRIPT_FUNCTION(script, onUpdate);
            }
        }
    }

    void ScriptEngine::onFixedUpdate() {
        if (dllLoaded) {
            for (Script* script: scripts) {
                //RUN_SCRIPT_FUNCTION(script, onFixedUpdate);
                script->onFixedUpdate();
            }
        }
    }


    Script* ScriptEngine::getScript(const std::string& name) {
        for (Script* script: scripts) {
            if (script->getName() == name) {
                return script;
            }
        }
        TE_LOGGER_ERROR("Script with name: " + name + " not found. Returning nullptr.");
        return nullptr;
    }


    void ScriptEngine::registerScript(Script* script) {
        script->name = typeid(*script).name();
        script->name.replace(0, 6, "");
        scripts.push_back(script);
    }

    void ScriptEngine::deleteScripts() {
        for (Script* script: scripts) {
            delete script;
        }
        scripts.clear();
    }
}
