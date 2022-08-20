#pragma once

#include "SocketHandler.hpp"
#include "ConnectionHandler.hpp"
#include "PacketHandler.hpp"
#include "../../dataStructure/Queue.hpp"

namespace TechEngineCore {

    class SocketHandler;

    class ConnectionHandler;

    class PacketHandler;

    class NetworkHandler {
    private:
    protected:
        std::string ip;
        short port;
        SocketHandler *socketHandler;
        ConnectionHandler *connectionHandler;
        PacketHandler *packetHandler;

        bool running = true;

    public:
        NetworkHandler(std::string &ip, const short &port, SocketHandler *socketsHandler, ConnectionHandler *connectionHandler, PacketHandler *packetHandler);

        ~NetworkHandler();

        SocketHandler *getSocketsHandler();

        ConnectionHandler &getConnectionHandler();

        PacketHandler &getPacketHandler();

        virtual void init() = 0;

        std::string &getIP();

        short getPort();

        bool isRunning();

        void setRunning(const bool &running);

        void update();
    };
}
