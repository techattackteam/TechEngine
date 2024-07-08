#pragma once

#include "core/ServerExportDll.hpp"
#include "scriptingAPI/TechEngineAPI.hpp"
#include "server/ServerAPI.hpp"

namespace TechEngine {
    class SERVER_DLL TechEngineServerAPI : public TechEngineAPI {
    protected:
        ServerAPI* serverAPI;
        Server* server;
        Communicator* communicator;

    public:
        TechEngineServerAPI(SystemsRegistry& systemsRegistry,
                            Server* server,
                            Communicator* communicator);

        ~TechEngineServerAPI() override;

        void init() override;
    };
}
