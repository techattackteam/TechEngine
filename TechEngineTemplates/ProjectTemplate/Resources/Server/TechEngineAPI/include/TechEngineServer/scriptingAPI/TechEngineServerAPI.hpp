#pragma once
#include "server/ServerAPI.hpp"


namespace TechEngine {
    class TechEngineServerAPI {
    private:
        ServerAPI* serverAPI;

    public:
        TechEngineServerAPI(Server* server, Communicator* communicator);
    };
}
