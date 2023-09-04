#pragma once

#include "CNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"

namespace TechEngine {
    class CConnectionHandler : public TechEngine::ConnectionHandler {
    private:
        CNetworkHandler *networkHandler;
    public:
        explicit CConnectionHandler(CNetworkHandler *networkHandler);

        ~CConnectionHandler();

        void requestConnection();

        void connectToServer();

        void onConnectionSuccessEvent(TechEngine::ConnectionSuccessfulEvent *event);

        void onPingEvent(TechEngine::PingEvent *event);
    };
}
