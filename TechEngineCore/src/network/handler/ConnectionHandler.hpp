#pragma once


namespace TechEngineCore {
    class NetworkHandler;

    class ConnectionHandler {
    protected:
        NetworkHandler *networkHandler;
    public:
        ConnectionHandler(NetworkHandler *networkHandler);
    };
}
