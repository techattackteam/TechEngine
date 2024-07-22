#pragma once
#include "event/EventDispatcherAPI.hpp"
#include "material/MaterialManagerAPI.hpp"
#include "scene/SceneManagerAPI.hpp"
#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class CORE_DLL TechEngineAPI {
    protected:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        MaterialManagerAPI* materialManagerAPI;
        SystemsRegistry& systemsRegistry;

    public:
        TechEngineAPI(SystemsRegistry& systemsRegistry);

        virtual ~TechEngineAPI();

        virtual void init();
    };
}
