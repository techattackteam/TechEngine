#pragma once

#include "CNetworkHandler.hpp"
#include "event/events/connection/PingEvent.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"

namespace TechEngine {
    class CConnectionHandler : public TechEngineCore::ConnectionHandler {
    private:
        CNetworkHandler *networkHandler;
    public:
        explicit CConnectionHandler(CNetworkHandler *networkHandler);

        ~CConnectionHandler();

        void requestConnection();

        void connectToServer();

        void onConnectionSuccessEvent(TechEngineCore::ConnectionSuccessfulEvent *event);

        void onPingEvent(TechEngineCore::PingEvent *event);
    };
}
