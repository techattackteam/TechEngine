#pragma once

#include <list>
#include <filesystem>
#include "Script.hpp"

namespace TechEngine {
    class ScriptEngine {
    private:
        std::list<Script *> scripts = {};
        inline static ScriptEngine *instance;

    public:
        ScriptEngine();

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        static ScriptEngine *getInstance();

        void registerScript(Script *pScript);

        void deleteScripts();

    private:

        void onInit();
    };
}

#define RegisterScript(Type)                                                \
namespace {                                                                 \
    TechEngine::Script *createScript() {                                    \
        TechEngine::Script *script = new Type();                            \
        TechEngine::ScriptEngine::getInstance()->registerScript(script);    \
        return script;                                                      \
    }                                                                       \
                                                                            \
    TechEngine::Script *registery = createScript();                         \
};