#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"
#include "event/events/connection/DisconnectionRequestEvent.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace TechEngineServer {
    class SConnectionHandler : public ConnectionHandler {
    private:
        SNetworkHandler *networkHandler;

        std::thread *isAlive;

        boost::uuids::random_generator uuidGenerator;

    public:
        explicit SConnectionHandler(SNetworkHandler *networkHandler);

        ~SConnectionHandler();

        void onConnectionRequest(ConnectionRequestEvent *event);

        void onDisconnectionRequest(DisconnectionRequestEvent *event);

        void checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp);

        void checkAliveClients();

        void onPingEvent(PingEvent *event);

        void timeoutClient(Client *client);

    };
}
