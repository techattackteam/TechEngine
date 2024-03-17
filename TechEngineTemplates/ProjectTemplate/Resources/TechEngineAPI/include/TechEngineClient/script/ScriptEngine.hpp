#pragma once

#include <list>
#include <filesystem>
#include <windows.h>

#include "Script.hpp"

namespace TechEngine {
    class ScriptEngine {
    private:
        std::list<Script*> scripts = {};
        inline static ScriptEngine* instance;

        HINSTANCE m_userCustomDll = nullptr;

    public:
        ScriptEngine();

        void init(const std::string& dllPath);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        static ScriptEngine* getInstance();

        static void registerScript(Script* script);

        void deleteScripts();

        void stop();
    };
}
