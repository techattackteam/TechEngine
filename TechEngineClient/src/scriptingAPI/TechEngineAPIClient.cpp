#include "TechEngineAPIClient.hpp"

namespace TechEngine {
    TechEngineAPIClient::TechEngineAPIClient(NetworkEngine* networkEngine) : networkAPI(new NetworkAPI(networkEngine)) {
    }

    TechEngineAPIClient::~TechEngineAPIClient() {
        delete networkAPI;
    }
}
