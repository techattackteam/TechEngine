#include "TechEngineAPI.hpp"

#include "material/MaterialManager.hpp"

namespace TechEngine {
    TechEngineAPI::TechEngineAPI(SceneManager* sceneManager,
                                 MaterialManager* materialManager) : sceneManagerAPI(new SceneManagerAPI(sceneManager)),
                                                                     eventDispatcherAPI(new EventDispatcherAPI()),
                                                                     materialManagerAPI(new MaterialManagerAPI(materialManager)) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
        delete eventDispatcherAPI;
        delete materialManagerAPI;
    }
}
