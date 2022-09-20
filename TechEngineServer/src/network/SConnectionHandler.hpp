#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"

namespace TechEngineServer {
    class SConnectionHandler : public ConnectionHandler {
    private:
        SNetworkHandler *networkHandler;

        std::thread *isAlive;
    public:
        explicit SConnectionHandler(SNetworkHandler *networkHandler);

        ~SConnectionHandler();

        void handleConnectionRequest(ConnectionRequestEvent *event);

        //void handleDisconnections(DisconnectionEvent *event);

        void checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp);

        void checkAliveClients();

        void onPingEvent(PingEvent *event);
    };
}
