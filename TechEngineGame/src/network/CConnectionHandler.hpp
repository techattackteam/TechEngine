#pragma once

#include "tegpph.hpp"
#include "CNetworkHandler.hpp"

namespace TechEngine {
    class CConnectionHandler : public ConnectionHandler {
    private:
        CNetworkHandler *networkHandler;
    public:
        explicit CConnectionHandler(CNetworkHandler *networkHandler);

        ~CConnectionHandler();

        void requestConnection();

        void connectToServer();

        //void onPingEvent(PingEvent *event);
    };
}
