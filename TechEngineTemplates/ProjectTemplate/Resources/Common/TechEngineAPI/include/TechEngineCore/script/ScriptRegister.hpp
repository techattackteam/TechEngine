#pragma once

#include <list>
#include <string>

namespace TechEngine {
    class ScriptEngine;
    class Script;

    class ScriptRegister {
    private:
        ScriptEngine* scriptEngine;
        inline static ScriptRegister* instance;

        std::list<std::string> scriptsNames;

    public:
        ScriptRegister();

        ScriptRegister(const ScriptRegister&) = delete;

        ScriptRegister& operator=(const ScriptRegister&) = delete;

        ~ScriptRegister() = default;

        void init(ScriptEngine* scriptEngine);

        static ScriptRegister* getInstance();

        static void registerScript(Script* script);
    };
}
