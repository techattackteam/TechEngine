#pragma once
#include "core/ClientExportDll.hpp"
#include "network/NetworkAPI.hpp"
#include "scriptingAPI/TechEngineAPI.hpp"

namespace TechEngine {
    class CLIENT_DLL TechEngineAPIClient : public TechEngineAPI {
    protected:
        NetworkAPI* networkAPI;

    public:
        TechEngineAPIClient(SceneManager* sceneManager, MaterialManager* materialManager, EventDispatcher* eventDispatcher, NetworkEngine* networkEngine);

        ~TechEngineAPIClient();
    };
}
