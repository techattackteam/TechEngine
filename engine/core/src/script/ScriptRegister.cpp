#include "ScriptRegister.hpp"

#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"
#include <iostream>

namespace TechEngine {
    ScriptRegister::ScriptRegister() {
        instance = this;
    }

    ScriptRegister::~ScriptRegister() {
        this->scriptEngine = nullptr;
        instance = nullptr;
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
