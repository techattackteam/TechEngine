#include "TechEngineAPI.hpp"

#include "material/MaterialManager.hpp"

namespace TechEngine {
    TechEngineAPI::TechEngineAPI(SceneManager* sceneManager,
                                 EventDispatcher* eventDispatcher,
                                 /*ScriptEngine* scriptEngine,*/
                                 MaterialManager* materialManager) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                     eventDispatcherAPI(new EventDispatcherAPI(eventDispatcher)),
                                                                     /*scriptEngineAPI(new ScriptEngineAPI(scriptEngine)),*/
                                                                     materialManagerAPI(new MaterialManagerAPI(materialManager)) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
    }
}
