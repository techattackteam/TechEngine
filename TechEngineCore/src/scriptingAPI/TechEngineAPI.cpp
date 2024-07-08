#include "TechEngineAPI.hpp"

#include "scene/SceneManager.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {
    TechEngineAPI::TechEngineAPI(SystemsRegistry& systemsRegistry) : systemsRegistry(systemsRegistry) {
    }


    TechEngineAPI::~TechEngineAPI() {
        delete sceneManagerAPI;
        delete eventDispatcherAPI;
        delete materialManagerAPI;
    }

    void TechEngineAPI::init() {
        sceneManagerAPI = new SceneManagerAPI(&systemsRegistry.getSystem<SceneManager>());
        eventDispatcherAPI = new EventDispatcherAPI(&systemsRegistry.getSystem<EventDispatcher>());
        materialManagerAPI = new MaterialManagerAPI(&systemsRegistry.getSystem<MaterialManager>());
    }
}
