#pragma once

namespace TechEngine {
    class Script {
    public:
        Script();

        virtual ~Script();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onClose();
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
