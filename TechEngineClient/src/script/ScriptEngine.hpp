#pragma once

#include <list>
#include <filesystem>
#include <windows.h>

#include "C:/dev/TechEngine/TechEngineClient/src/ScriptingAPI/scripts/Script.hpp"
namespace TechEngine {
    class ScriptEngine {
    private:
        std::list<Script *> scripts = {};
        inline static ScriptEngine* instance;

        HINSTANCE m_userCustomDll = nullptr;

    public:
        ScriptEngine();

        void init(const std::string&dllPath);

        void onStart();

        void onUpdate();

        void onFixedUpdate();

        static ScriptEngine* getInstance();

        void registerScript(Script* pScript);

        void deleteScripts();

        void stop();
    };
}
