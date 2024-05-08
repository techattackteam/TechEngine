#include "TechEngineAPI.hpp"

#include "material/MaterialManager.hpp"

namespace TechEngine {
    TechEngineAPI::TechEngineAPI(SceneManager* sceneManager,
                                 EventDispatcher* eventDispatcher,
                                 MaterialManager* materialManager) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                     eventDispatcherAPI(new EventDispatcherAPI(eventDispatcher)),
                                                                     materialManagerAPI(new MaterialManagerAPI(materialManager)) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
        delete eventDispatcherAPI;
        delete materialManagerAPI;
    }
}
