#pragma once
#include "scriptingAPI/TechEngineAPI.hpp"
#include "server/ServerAPI.hpp"


namespace TechEngine {
    class TechEngineServerAPI : public TechEngineAPI {
    protected:
        ServerAPI* serverAPI;

    public:
        TechEngineServerAPI(SceneManager* sceneManager,
                            MaterialManager* materialManager,
                            EventDispatcher* eventDispatcher,
                            Server* server,
                            Communicator* communicator);
    };
}
