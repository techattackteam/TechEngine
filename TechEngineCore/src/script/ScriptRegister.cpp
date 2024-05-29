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
        std::cout << "ScriptRegister instance at " << (void*)instance << std::endl;
        return instance;
    }

    void ScriptRegister::registerScript(Script* script) {
        ScriptEngine* clientScriptEngine = getInstance()->scriptEngine;
        clientScriptEngine->registerScript(script);
    }
}
