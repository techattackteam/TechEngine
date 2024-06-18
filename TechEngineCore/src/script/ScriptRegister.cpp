#include "ScriptRegister.hpp"

#include <iostream>

#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"
#include "script/Script.hpp"

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
        std::string string = "ScriptRegister instance at " + std::to_string((long long)instance);
        TE_LOGGER_INFO(string);
        return instance;
    }

    void ScriptRegister::registerScript(Script* script) {
        getInstance()->scriptEngine->registerScript(script);
    }
}
