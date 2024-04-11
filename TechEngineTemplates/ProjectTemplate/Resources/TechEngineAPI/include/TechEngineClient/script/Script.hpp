#pragma once
#include <string>

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

#define RegisterScript(Type)                                    \
    namespace {                                                 \
        TechEngine::Script *createScript() {                    \
            TechEngine::Script *script = new Type();            \
            TechEngine::ScriptEngine::registerScript(script);   \
            return script;                                      \
    }                                                           \
                                                                \
    TechEngine::Script *registery = createScript();             \
};
