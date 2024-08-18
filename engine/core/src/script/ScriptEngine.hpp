#pragma once

#include "Script.hpp"
#include "ScriptRegister.hpp"
#include "systems/System.hpp"

#include <Windows.h>
#include <list>
#include <utility>

namespace TechEngine {
    class CORE_DLL ScriptEngine : public System {
    private:
        SystemsRegistry& m_systemRegistry;
        ScriptRegister* scriptRegister = nullptr;
        std::list<Script*> scripts = {};
        bool runtime = false;
        bool dllLoaded = false;
        bool loadingScripts = false;

        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

        inline static std::function<void(SystemsRegistry*)> m_APIEntryPoint = nullptr;

    public:
        explicit ScriptEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        void loadDLL(const std::string& dllPath);

        void shutdown() override;

        void stop();

        void onStart() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        bool isLoadingScripts() const {
            return loadingScripts;
        }

        static void setEntryPoint(std::function<void(SystemsRegistry*)> entryPoint) {
            m_APIEntryPoint = std::move(entryPoint);
        }
    };
}
