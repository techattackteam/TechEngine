#include "TechEngineAPIClient.hpp"

namespace TechEngine {
    TechEngineAPIClient::TechEngineAPIClient(SceneManager* sceneManager,
                                             MaterialManager* materialManager,
                                             EventDispatcher* eventDispatcher,
                                             NetworkEngine* networkEngine) : networkAPI(new NetworkAPI(networkEngine)),
                                                                             TechEngineAPI(sceneManager,
                                                                                           materialManager,
                                                                                           eventDispatcher) {
    }

    TechEngineAPIClient::~TechEngineAPIClient() {
        delete networkAPI;
    }
}
