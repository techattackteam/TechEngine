#pragma once
#include <string>


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

        Script* getScript(const std::string& name);

        static void registerScript(Script* script);
    };
}
