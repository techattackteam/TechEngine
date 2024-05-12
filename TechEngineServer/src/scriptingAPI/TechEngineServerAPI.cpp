#include "TechEngineServerAPI.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    TechEngineServerAPI::TechEngineServerAPI(SceneManager* sceneManager,
                                             MaterialManager* materialManager,
                                             EventDispatcher* eventDispatcher,
                                             Server* server,
                                             Communicator* communicator) : serverAPI(new ServerAPI(server, communicator)),
                                                                           TechEngineAPI(sceneManager,
                                                                                         materialManager,
                                                                                         eventDispatcher) {
    }
}
