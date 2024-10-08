#pragma once

#include "common/include/core/ExportDLL.hpp"

#include <string>
#include <vcruntime_typeinfo.h>

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

        void setName(const std::string& name);

        const std::string& getName() const;
    };
}

#define RegisterScript(Type)                            \
namespace {                                             \
    TechEngineAPI::Script* createScript() {             \
        TechEngineAPI::Script*script = new Type();      \
        script->setName(#Type);                        \
        return script;                                  \
    }                                                   \
                                                        \
    TechEngineAPI::Script*registery = createScript();   \
};
