#pragma once

#include <list>
#include <filesystem>
#include "Script.hpp"
#include <windows.h>

namespace TechEngine {
    class ScriptEngine {
    private:
        std::list<Script *> scripts = {};
        inline static ScriptEngine *instance;

        HINSTANCE m_userCustomDll = nullptr;

    public:
        ScriptEngine();

        void init(const std::string &dllPath);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        static ScriptEngine *getInstance();

        void registerScript(Script *pScript);

        void deleteScripts();

        void stop();
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