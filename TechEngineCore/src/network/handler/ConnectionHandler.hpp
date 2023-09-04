#pragma once


namespace TechEngine {
    class NetworkHandler;

    class ConnectionHandler {
    protected:
        NetworkHandler *networkHandler;
    public:
        ConnectionHandler(NetworkHandler *networkHandler);
    };
}
