#pragma once
#include "network/NetworkAPI.hpp"

namespace TechEngine {
    class TechEngineAPIClient {
    private:
        NetworkAPI* networkAPI;

    public:
        TechEngineAPIClient(NetworkEngine* networkEngine);

        ~TechEngineAPIClient();
    };
}
