#pragma once

#include <iostream>

#include "Script.hpp"
#include "ScriptRegister.hpp"
#include "systems/System.hpp"

#include <Windows.h>
#include <list>
#include <utility>

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

        std::function<spdlog::sinks::dist_sink_mt*(SystemsRegistry*)> m_APIEntryPoint = nullptr;

    public:
        inline static ScriptEngine* instance = nullptr;

        explicit ScriptEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        std::tuple<bool, spdlog::sinks::dist_sink_mt*> loadDLL(const std::string& dllPath);

        void shutdown() override;

        void stop();

        void onStart() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        static void setEntryPoint(std::function<spdlog::sinks::dist_sink_mt*(SystemsRegistry*)> entryPoint) {
            instance->m_APIEntryPoint = std::move(entryPoint);
        }
    };
}
