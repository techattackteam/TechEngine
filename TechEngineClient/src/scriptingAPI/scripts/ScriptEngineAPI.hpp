#pragma once


namespace TechEngine {
    class Script;
    class ScriptEngine;
}

namespace TechEngine {
    class ScriptEngineAPI {
    private:
        inline static ScriptEngine* scriptEngine = nullptr;

    public:
        explicit ScriptEngineAPI(ScriptEngine* scriptEngine);

        ~ScriptEngineAPI();

        static void registerScript(Script* script);
    };
}
