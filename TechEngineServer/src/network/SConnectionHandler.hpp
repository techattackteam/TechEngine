#pragma once

#include "tespph.hpp"
#include "SNetworkHandler.hpp"

namespace TechEngineServer {
    class SConnectionHandler : public ConnectionHandler {
    private:
        SNetworkHandler *networkHandler;
    public:
        explicit SConnectionHandler(SNetworkHandler *networkHandler);

        ~SConnectionHandler();

        void handleConnectionRequest(ConnectionRequestEvent *event);

        //void handleDisconnections(DisconnectionEvent *event);

        void checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp);

        void checkAliveClients();

        //void onPingResponse(PingResponseEvent *event);
    };
}
