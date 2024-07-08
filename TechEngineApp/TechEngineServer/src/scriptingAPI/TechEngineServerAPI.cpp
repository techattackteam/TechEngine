#include "TechEngineServerAPI.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    TechEngineServerAPI::TechEngineServerAPI(SystemsRegistry& systemsRegistry,
                                             Server* server,
                                             Communicator* communicator): server(server),
                                                                          communicator(communicator),
                                                                          TechEngineAPI(systemsRegistry) {
    }

    TechEngineServerAPI::~TechEngineServerAPI() {
        delete serverAPI;
    }

    void TechEngineServerAPI::init() {
        TechEngineAPI::init();
        serverAPI = new ServerAPI(server, communicator);
    }
}
