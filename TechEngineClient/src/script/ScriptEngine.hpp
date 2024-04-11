#pragma once

#include <list>
#include <filesystem>
#include <windows.h>

#include "Script.hpp"

namespace TechEngine {
    class ScriptEngine {
    private:
        std::list<Script*> scripts = {};
        bool runtime = false;
        inline static ScriptEngine* instance;

        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

    public:
        ScriptEngine(bool runtime);

        void init(const std::string& dllPath);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        void onFixedUpdateLoop();

        Script* getScript(const std::string& name);

        static ScriptEngine* getInstance();

        static void registerScript(Script* script);

        void deleteScripts();

        void stop();
    };
}
