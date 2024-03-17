#pragma once
#include "event/EventDispatcherAPI.hpp"
#include "material/MaterialManagerAPI.hpp"
#include "scene/SceneManagerAPI.hpp"
#include "scripts/ScriptEngineAPI.hpp"


namespace TechEngine {
    class TechEngineAPI {
    private:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        ScriptEngineAPI* scriptEngineAPI;
        MaterialManagerAPI* materialManagerAPI;

    public:
        TechEngineAPI(SceneManager* sceneManager, EventDispatcher* eventDispatcher, /*ScriptEngine* scriptEngine,*/ MaterialManager* materialManager);

        ~TechEngineAPI();
    };
}
