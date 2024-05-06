#pragma once
#include <string>
#include <filesystem>

namespace TechEngine {
    class Script {
    private:
        std::string name;

        friend class ScriptEngine;

    public:
        Script();

        virtual ~Script();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onClose();

        const std::string& getName() const {
            return name;
        }
    };
}

#define RegisterScript(Type)                                                                    \
    namespace {                                                                                 \
        TechEngine::Script *createScript() {                                                    \
            TechEngine::Script *script = new Type();                                            \
            TechEngine::ScriptEngine::registerScript(script, std::filesystem::current_path());  \
            return script;                                                                      \
    }                                                                                           \
                                                                                                \
    TechEngine::Script *registery = createScript();                                             \
};
