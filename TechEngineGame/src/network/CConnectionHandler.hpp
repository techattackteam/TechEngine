#pragma once

#include "tegpph.hpp"
#include "CNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"

namespace TechEngine {
    class CConnectionHandler : public ConnectionHandler {
    private:
        CNetworkHandler *networkHandler;
    public:
        explicit CConnectionHandler(CNetworkHandler *networkHandler);

        ~CConnectionHandler();

        void requestConnection();

        void connectToServer();

        void onConnectionSuccessEvent(ConnectionSuccessfulEvent *event);

        void onPingEvent(PingEvent *event);
    };
}
