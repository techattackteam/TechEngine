#include "ScriptEngineAPI.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngine {
    ScriptEngineAPI::ScriptEngineAPI(TechEngine::ScriptEngine* scriptEngine) {
        ScriptEngineAPI::scriptEngine = scriptEngine;
    }

    ScriptEngineAPI::~ScriptEngineAPI() {
    }

    Script* ScriptEngineAPI::getScript(const std::string& name) {
        return scriptEngine->getScript(name);
    }

    void ScriptEngineAPI::registerScript(TechEngine::Script* script) {
        scriptEngine->registerScript(script);
    }
}
