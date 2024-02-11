#pragma once

namespace TechEngine {
    class Script {
    protected:

    public:
        Script();

        virtual ~Script();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onClose();
    };
}

#define RegisterScript(Type)                                                                               \
    namespace {                                                                                            \
        TechEngine::Script *createScript() {                                                            \
            TechEngine::Script *script = new Type();                                                    \
            TechEngineAPI::ScriptEngineAPI::registerScript(script);                                        \
            return script;                                                                                 \
    }                                                                                                      \
                                                                                                           \
    TechEngine::Script *registery = createScript();                                                     \
};
