#pragma once

#include "scene/SceneManagerAPI.hpp"
#include "event/EventDispatcherAPI.hpp"
#include "material/MaterialManagerAPI.hpp"
#include "scripts/ScriptEngineAPI.hpp"

namespace TechEngineAPI {
    class TechEngineAPI {
    private:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        ScriptEngineAPI* scriptEngineAPI;
        MaterialManagerAPI* materialManagerAPI;

    public:
        TechEngineAPI(TechEngine::SceneManager* sceneManager, TechEngine::EventDispatcher* eventDispatcher, TechEngine::ScriptEngine* scriptEngine, TechEngine::MaterialManager* materialManager);

        ~TechEngineAPI();
    };
}
