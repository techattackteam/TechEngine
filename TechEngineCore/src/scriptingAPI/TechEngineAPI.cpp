#include "TechEngineAPI.hpp"

#include "../material/MaterialManager.hpp"
#include "material/MaterialManagerAPI.hpp"

namespace TechEngine {
    TechEngineAPI::TechEngineAPI(SceneManager* sceneManager,
                                 MaterialManager* materialManager,
                                 EventDispatcher* eventDispatcher) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                     eventDispatcherAPI(new EventDispatcherAPI(eventDispatcher)),
                                                                     materialManagerAPI(new MaterialManagerAPI(materialManager)) {
    }

    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
        delete eventDispatcherAPI;
        delete materialManagerAPI;
    }
}
