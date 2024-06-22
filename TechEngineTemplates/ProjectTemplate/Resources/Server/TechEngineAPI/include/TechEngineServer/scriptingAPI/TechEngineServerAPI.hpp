#pragma once

#include "core/ServerExportDll.hpp"
#include "scriptingAPI/TechEngineAPI.hpp"
#include "server/ServerAPI.hpp"

namespace TechEngine {
    class SERVER_DLL TechEngineServerAPI : public TechEngineAPI {
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
