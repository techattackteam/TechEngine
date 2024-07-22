#pragma once

#include "script/ScriptRegister.hpp"


namespace TechEngine {
    class CORE_DLL Script {
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

        const std::string& getName() const;
    };
}

#define RegisterScript(Type)                                                    \
    namespace {                                                                 \
        TechEngine::Script *createScript() {                                    \
            TechEngine::Script *script = new Type();                            \
            TechEngine::ScriptRegister::registerScript(script);                 \
            return script;                                                      \
    }                                                                           \
                                                                                \
    TechEngine::Script *registery = createScript();                             \
};
