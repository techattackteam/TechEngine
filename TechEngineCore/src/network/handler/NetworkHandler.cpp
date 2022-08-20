#include "NetworkHandler.hpp"

namespace TechEngineCore {

    NetworkHandler::NetworkHandler(std::string &ip, const short &port, SocketHandler *socketHandler, ConnectionHandler *connectionHandler, PacketHandler *packetHandler) {
        running = true;
        this->ip = ip;
        this->port = port;
        this->socketHandler = socketHandler;
        this->connectionHandler = connectionHandler;
        this->packetHandler = packetHandler;
    }

    NetworkHandler::~NetworkHandler() {
        running = false;
        delete (socketHandler);
        delete (connectionHandler);
        delete (packetHandler);
    }

    SocketHandler *NetworkHandler::getSocketsHandler() {
        return socketHandler;
    }

    ConnectionHandler &NetworkHandler::getConnectionHandler() {
        return *connectionHandler;
    }

    PacketHandler &NetworkHandler::getPacketHandler() {
        return *packetHandler;
    }

    std::string &NetworkHandler::getIP() {
        return ip;
    }

    short NetworkHandler::getPort() {
        return port;
    }

    bool NetworkHandler::isRunning() {
        return running;
    }

    void NetworkHandler::setRunning(const bool &running) {
        this->running = running;
    }

    void NetworkHandler::update() {
        packetHandler->processIncomePacket();
    }
}
