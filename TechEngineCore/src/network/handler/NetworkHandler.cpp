#include "NetworkHandler.hpp"

namespace TechEngine {

    NetworkHandler::NetworkHandler(SocketHandler *socketHandler, ConnectionHandler *connectionHandler, PacketHandler *packetHandler) {
        running = true;
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
