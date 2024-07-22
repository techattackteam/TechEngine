#pragma once

#include <list>
#include <filesystem>
#include <windows.h>

#include "Script.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    class CORE_DLL ScriptEngine : public System {
    private:
        std::list<Script*> scripts = {};
        bool runtime = false;
        bool dllLoaded = false;
        bool loadingScripts = false;

        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

        EventDispatcher* eventDispatcher = nullptr;

    public:
        explicit ScriptEngine(bool runtime = false);

        void init(const std::string& dllPath, EventDispatcher* eventDispatcher);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        void stop();

        bool isLoadingScripts() const {
            return loadingScripts;
        }
    };
}
