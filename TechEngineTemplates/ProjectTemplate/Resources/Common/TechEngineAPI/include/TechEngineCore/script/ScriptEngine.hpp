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
        bool dllLoaded = false;

        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

    public:
        explicit ScriptEngine(bool runtime);

        void init(const std::string& dllPath);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        void stop();
    };
}
