#pragma once

#include "SocketHandler.hpp"
#include "ConnectionHandler.hpp"
#include "PacketHandler.hpp"
#include "dataStructure/Queue.hpp"

namespace TechEngine {

    class SocketHandler;

    class ConnectionHandler;

    class PacketHandler;

    class NetworkHandler {
    private:
    protected:
        SocketHandler *socketHandler;
        ConnectionHandler *connectionHandler;
        PacketHandler *packetHandler;

        std::atomic_bool running = true;

    public:
        NetworkHandler(SocketHandler *socketsHandler, ConnectionHandler *connectionHandler, PacketHandler *packetHandler);

        ~NetworkHandler();

        SocketHandler *getSocketsHandler();

        ConnectionHandler &getConnectionHandler();

        PacketHandler &getPacketHandler();

        virtual void init() = 0;

        bool isRunning();

        void setRunning(const bool &running);

        void update();
    };
}
