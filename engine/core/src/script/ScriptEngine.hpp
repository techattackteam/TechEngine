#pragma once

#include <Windows.h>
#include <list>
#include <utility>

namespace TechEngine {
    class Script;
    class ScriptRegister;

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
        
        HINSTANCE m_userCustomDll = nullptr;
        HANDLE m_dllProcessHandle = nullptr;
        HANDLE m_dllThreadHandle = nullptr;

        std::function<spdlog::sinks::dist_sink_mt*(SystemsRegistry*)> m_APIEntryPoint = nullptr;
        std::function<void()> m_updateComponentAPIsFunction = nullptr;
        std::function<void()> m_updateComponentsFromAPIsFunction = nullptr;

    public:
        inline static ScriptEngine* instance = nullptr;

        explicit ScriptEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        void shutdown() override;

        std::tuple<bool, spdlog::sinks::dist_sink_mt*> start(const std::string& dllPath);

        void stop();

        void onStart() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        void onStop() override;

        Script* getScript(const std::string& name);

        void registerScript(Script* script);

        void deleteScripts();

        static void setEntryPoint(std::function<spdlog::sinks::dist_sink_mt*(SystemsRegistry*)> entryPoint) {
            instance->m_APIEntryPoint = std::move(entryPoint);
        }

        static void setUpdateComponentAPIsFunction(std::function<void()> updateComponentsFunction) {
            instance->m_updateComponentAPIsFunction = std::move(updateComponentsFunction);
        }

        static void setUpdateComponentsFromAPIsFunction(std::function<void()> updateComponentsFromAPIsFunction) {
            instance->m_updateComponentsFromAPIsFunction = std::move(updateComponentsFromAPIsFunction);
        }
    };
}
