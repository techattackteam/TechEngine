#include "TechEngineAPI.hpp"

namespace TechEngineAPI {
    TechEngineAPI::TechEngineAPI(TechEngine::SceneManager* sceneManager,
                                 TechEngine::EventDispatcher* eventDispatcher,
                                 TechEngine::ScriptEngine* scriptEngine) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                           eventDispatcherAPI(new EventDispatcherAPI(eventDispatcher)),
                                                                                              scriptEngineAPI(new ScriptEngineAPI(scriptEngine)) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
    }
}
