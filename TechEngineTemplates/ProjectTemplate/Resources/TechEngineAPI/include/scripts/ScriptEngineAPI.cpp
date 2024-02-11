#include "ScriptEngineAPI.hpp"
#include "script/ScriptEngine.hpp"

namespace TechEngineAPI {
    ScriptEngineAPI::ScriptEngineAPI(TechEngine::ScriptEngine* scriptEngine) {
        ScriptEngineAPI::scriptEngine = scriptEngine;
    }

    ScriptEngineAPI::~ScriptEngineAPI() {
    }

    void ScriptEngineAPI::registerScript(TechEngine::Script* script) {
        scriptEngine->registerScript(script);
    }
}
