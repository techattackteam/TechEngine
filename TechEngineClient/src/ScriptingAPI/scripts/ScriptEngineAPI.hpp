#pragma once


namespace TechEngine {
    class Script;
    class ScriptEngine;
}

namespace TechEngineAPI {
    class ScriptEngineAPI {
    private:
        inline static TechEngine::ScriptEngine* scriptEngine = nullptr;

    public:
        explicit ScriptEngineAPI(TechEngine::ScriptEngine* scriptEngine);

        ~ScriptEngineAPI();

        static void registerScript(TechEngine::Script* script);
    };
}
