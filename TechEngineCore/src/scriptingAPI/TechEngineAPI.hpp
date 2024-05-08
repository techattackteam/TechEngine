#pragma once
#include "event/EventDispatcherAPI.hpp"
#include "material/MaterialManagerAPI.hpp"
#include "scene/SceneManagerAPI.hpp"

namespace TechEngine {
    class TechEngineAPI {
    private:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        MaterialManagerAPI* materialManagerAPI;

    public:
        TechEngineAPI(SceneManager* sceneManager, EventDispatcher* eventDispatcher, MaterialManager* materialManager);

        ~TechEngineAPI();
    };
}
