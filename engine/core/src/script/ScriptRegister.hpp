#pragma once

#include "core/CoreExportDll.hpp"
#include <list>
#include <string>

namespace TechEngine {
    class ScriptEngine;
    class Script;

    class CORE_DLL ScriptRegister {
    private:
        ScriptEngine* scriptEngine;
        inline static ScriptRegister* instance;

        std::list<std::string> scriptsNames;

    public:
        ScriptRegister();


        ~ScriptRegister();

        void init(ScriptEngine* scriptEngine);

        static ScriptRegister* getInstance();

        static void registerScript(Script* script);
    };
}