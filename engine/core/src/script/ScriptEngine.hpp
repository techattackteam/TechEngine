#pragma once

#include "Script.hpp"
#include "ScriptRegister.hpp"
#include "systems/System.hpp"

#include <Windows.h>
#include <list>
#include <utility>

#include "project/ProjectManager.hpp"

namespace TechEngine {
    enum class CompileMode {
        Debug,
        Release
    };

    class CORE_DLL ScriptEngine : public System {
    private:
        SystemsRegistry& m_systemRegistry;
        ScriptRegister* scriptRegister = nullptr;
        std::list<Script*> scripts = {};
        bool runtime = false;
        bool dllLoaded = false;

        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

        inline static std::function<void(SystemsRegistry*)> m_APIEntryPoint = nullptr;

    public:
        explicit ScriptEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        bool loadDLL(const std::string& dllPath);

        void shutdown() override;

        void stop();

        void onStart() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        static void setEntryPoint(std::function<void(SystemsRegistry*)> entryPoint) {
            m_APIEntryPoint = std::move(entryPoint);
        }
    };
}
