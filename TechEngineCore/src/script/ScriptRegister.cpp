#include "ScriptRegister.hpp"

#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"
#include "script/Script.hpp"
#include <iostream>

namespace TechEngine {
    ScriptRegister::ScriptRegister() {
        instance = this;
    }

    void ScriptRegister::init(ScriptEngine* scriptEngine) {
        this->scriptEngine = scriptEngine;
    }

    ScriptRegister* ScriptRegister::getInstance() {
        if (instance == nullptr) {
            instance = new ScriptRegister();
        }
        return instance;
    }

    void ScriptRegister::registerScript(Script* script) {
        getInstance()->scriptEngine->registerScript(script);
    }
}
