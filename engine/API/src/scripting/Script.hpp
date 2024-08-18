#pragma once
#include <string>
#include "core/ExportDLL.hpp"

namespace TechEngine {
    class Script;
}

namespace TechEngineAPI {
    class API_DLL Script {
    private:
        std::string name;
        TechEngine::Script* script;

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

#define RegisterScript(Type)                            \
namespace {                                             \
    TechEngineAPI::Script* createScript() {             \
        TechEngineAPI::Script*script = new Type();      \
        return script;                                  \
    }                                                   \
                                                        \
    TechEngineAPI::Script*registery = createScript();   \
};
