#include "TechEngineAPI.hpp"

#include "material/MaterialManager.hpp"

namespace TechEngineAPI {
    TechEngineAPI::TechEngineAPI(TechEngine::SceneManager* sceneManager,
                                 TechEngine::EventDispatcher* eventDispatcher,
                                 TechEngine::ScriptEngine* scriptEngine,
                                 TechEngine::MaterialManager* materialManager) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                                 eventDispatcherAPI(new EventDispatcherAPI(eventDispatcher)),
                                                                                 scriptEngineAPI(new ScriptEngineAPI(scriptEngine)),
                                                                                 materialManagerAPI(new MaterialManagerAPI(materialManager)) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
    }
}
