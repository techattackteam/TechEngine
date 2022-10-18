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

        std::string dllPath = std::filesystem::current_path().string() + "\\project\\scripts\\\\cmake-build-debug\\UserProject.dll";
    public:
        ScriptEngine();

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        static ScriptEngine *getInstance();

        void registerScript(Script *pScript);

        void deleteScripts();

        void init();


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