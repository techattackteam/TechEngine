#include <iostream>
#include "ScriptEngine.hpp"

namespace TechEngine {
    ScriptEngine::ScriptEngine() {
        if (instance != nullptr) {
            delete this;
        } else {
            instance = this;
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

    void ScriptEngine::onInit() {
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
