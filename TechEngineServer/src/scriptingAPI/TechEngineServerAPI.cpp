#include "TechEngineServerAPI.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    TechEngineServerAPI::TechEngineServerAPI(Server* server, Communicator* communicator) : serverAPI(new ServerAPI(server, communicator)) {
    }
}
