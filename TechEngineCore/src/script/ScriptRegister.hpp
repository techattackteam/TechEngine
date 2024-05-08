#pragma once

#include "ScriptEngine.hpp"

namespace TechEngine {
    class ScriptRegister {
    private:
        ScriptEngine* clientScriptEngine;
        ScriptEngine* serverScriptEngine;
        static ScriptRegister instance;

    public:
        ScriptRegister() = default;

        ScriptRegister(const ScriptRegister&) = delete;

        ScriptRegister& operator=(const ScriptRegister&) = delete;

        ~ScriptRegister() = default;

        void init(ScriptEngine* clientScriptEngine, ScriptEngine* serverScriptEngine);

        static ScriptRegister& getInstance();

        static void registerScript(Script* script, std::string path);
    };
}
