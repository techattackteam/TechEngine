#pragma once

#include "SNetworkHandler.hpp"
#include "events/connection/PingEvent.hpp"
#include "events/connection/DisconnectionRequestEvent.hpp"
#include "events/connection/ConnectionRequestEvent.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace TechEngineServer {
    class SConnectionHandler : public TechEngine::ConnectionHandler {
    private:
        SNetworkHandler *networkHandler;

        std::thread *isAlive;

        boost::uuids::random_generator uuidGenerator;

    public:
        explicit SConnectionHandler(SNetworkHandler *networkHandler);

        ~SConnectionHandler();

        void onConnectionRequest(TechEngine::ConnectionRequestEvent *event);

        void onDisconnectionRequest(TechEngine::DisconnectionRequestEvent *event);

        void checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp);

        void checkAliveClients();

        void onPingEvent(TechEngine::PingEvent *event);

        void timeoutClient(Client *client);

    };
}
