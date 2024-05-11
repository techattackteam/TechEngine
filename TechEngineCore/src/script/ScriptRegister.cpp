#include "ScriptRegister.hpp"

#include "core/Logger.hpp"
#include "script/ScriptEngine.hpp"
#include "script/Script.hpp"

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
        ScriptEngine* clientScriptEngine = getInstance().clientScriptEngine;
        ScriptEngine* serverScriptEngine = getInstance().serverScriptEngine;
        //Find first Assets in path
        size_t pos = path.find("\\Assets\\");
        if (pos != std::string::npos) {
            path = path.substr(pos + 8);
        }
        path = path.substr(0, path.find_first_of('\\'));
        if (path == "Client") {
            clientScriptEngine->registerScript(script);
        } else if (path == "Server") {
            serverScriptEngine->registerScript(script);
        } else if (path == "Common") {
            if (clientScriptEngine->isLoadingScripts()) {
                clientScriptEngine->registerScript(script);
            } else if (serverScriptEngine->isLoadingScripts()) {
                serverScriptEngine->registerScript(script);
            } else {
                TE_LOGGER_WARN("Client and Server script engines are not loading scripts. Skipping script registration.");
            }
        } else {
            TE_LOGGER_WARN("Client and Server script engines are not loading scripts. Skipping script registration.");
        }
    }
}
