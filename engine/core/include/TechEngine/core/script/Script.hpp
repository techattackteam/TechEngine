#pragma once

#include "TechEngine/core/core/CoreExportDLL.hpp"
#include <functional>
#include <string>


namespace TechEngine {
    class MeshManager;
    class MaterialManager;
    class Scene;

    class CORE_DLL Script {
    private:
        std::string m_name;
        friend class ScriptRegister;

    protected:
        Scene* m_scene;
        MaterialManager* m_materialManager;
        MeshManager* m_meshManager;

    public:
        Script();

        virtual ~Script();

        virtual void onStart();

        virtual void onFixedUpdate();

        virtual void onUpdate();

        virtual void onClose();

        void setName(const std::string& name);

        const std::string& getName() const;

        static Script* registerScript(Script* script);
    };
}

#define RegisterScript(Type)                        \
namespace {                                         \
TechEngine::Script* createScript() {                \
TechEngine::Script*script = new Type();             \
script->setName(#Type);                             \
return script;                                      \
}                                                   \
                                                    \
TechEngine::Script*registery = createScript();      \
};
