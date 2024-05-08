#include "ScriptRegister.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    ScriptRegister ScriptRegister::instance;

    void ScriptRegister::init(ScriptEngine* clientScriptEngine, ScriptEngine* serverScriptEngine) {
        this->clientScriptEngine = clientScriptEngine;
        this->serverScriptEngine = serverScriptEngine;
    }

    ScriptRegister& ScriptRegister::getInstance() {
        return instance;
    }

    void ScriptRegister::registerScript(Script* script, std::string path) {
        //Find first Assets in path
        size_t pos = path.find("\\Assets\\");
        if (pos != std::string::npos) {
            path = path.substr(pos + 8);
        }
        path = path.substr(0, path.find_first_of('\\'));
        if (path == "Client") {
            getInstance().clientScriptEngine->registerScript(script);
        } else if (path == "Server") {
            getInstance().serverScriptEngine->registerScript(script);
        } else {
            getInstance().clientScriptEngine->registerScript(script);
            getInstance().serverScriptEngine->registerScript(script);
        }
    }
}
