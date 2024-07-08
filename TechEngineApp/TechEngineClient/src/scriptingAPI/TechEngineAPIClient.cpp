#include "network/NetworkEngine.hpp"
#include "TechEngineAPIClient.hpp"

namespace TechEngine {
    TechEngineAPIClient::TechEngineAPIClient(SystemsRegistry& systemsRegistry) : TechEngineAPI(systemsRegistry) {
    }

    TechEngineAPIClient::~TechEngineAPIClient() {
        delete networkAPI;
    }

    void TechEngineAPIClient::init() {
        TechEngineAPI::init();
        networkAPI = new NetworkAPI(&systemsRegistry.getSystem<NetworkEngine>());
    }
}
