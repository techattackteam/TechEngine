#pragma once
#include "event/EventDispatcherAPI.hpp"
#include "material/MaterialManagerAPI.hpp"
#include "scene/SceneManagerAPI.hpp"
#include "core/CoreExportDll.hpp"

namespace TechEngine {
    class CORE_DLL TechEngineAPI {
    private:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        MaterialManagerAPI* materialManagerAPI;

    public:
        TechEngineAPI(SceneManager* sceneManager, MaterialManager* materialManager, EventDispatcher* eventDispatcher);

        ~TechEngineAPI();
    };
}
