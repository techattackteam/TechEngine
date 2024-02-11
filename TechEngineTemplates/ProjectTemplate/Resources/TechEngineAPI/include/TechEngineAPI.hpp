#pragma once

#include "scene/SceneManagerAPI.hpp"
#include "event/EventDispatcherAPI.hpp"
#include "scripts/ScriptEngineAPI.hpp"

namespace TechEngineAPI {
    class TechEngineAPI {
    private:
        SceneManagerAPI* sceneManagerAPI;
        EventDispatcherAPI* eventDispatcherAPI;
        ScriptEngineAPI* scriptEngineAPI;

    public:
        TechEngineAPI(TechEngine::SceneManager* sceneManager, TechEngine::EventDispatcher* eventDispatcher, TechEngine::ScriptEngine* scriptEngine);

        ~TechEngineAPI();
    };
}
