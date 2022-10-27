#pragma once

#include "SNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"
#include "event/events/connection/DisconnectionRequestEvent.hpp"
#include "event/events/connection/ConnectionRequestEvent.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace TechEngineServer {
    class SConnectionHandler : public TechEngineCore::ConnectionHandler {
    private:
        SNetworkHandler *networkHandler;

        std::thread *isAlive;

        boost::uuids::random_generator uuidGenerator;

    public:
        explicit SConnectionHandler(SNetworkHandler *networkHandler);

        ~SConnectionHandler();

        void onConnectionRequest(TechEngineCore::ConnectionRequestEvent *event);

        void onDisconnectionRequest(TechEngineCore::DisconnectionRequestEvent *event);

        void checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp);

        void checkAliveClients();

        void onPingEvent(TechEngineCore::PingEvent *event);

        void timeoutClient(Client *client);

    };
}
