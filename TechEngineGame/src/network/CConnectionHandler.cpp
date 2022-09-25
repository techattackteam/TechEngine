#include <iostream>
#include "CConnectionHandler.hpp"
#include "network/packets/connection/PingPacket.hpp"
#include "network/packets/connection/ConnectionRequestPacket.hpp"

namespace TechEngine {
    CConnectionHandler::CConnectionHandler(CNetworkHandler *networkHandler) : networkHandler(networkHandler), ConnectionHandler(networkHandler) {
        EventDispatcher::getInstance().subscribe(PingEvent::eventType, [this](Event *event) {
            onPingEvent((PingEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(ConnectionSuccessfulEvent::eventType, [this](Event *event) {
            onConnectionSuccessEvent((ConnectionSuccessfulEvent *) event);
        });
    }

    CConnectionHandler::~CConnectionHandler() {
    }

    void CConnectionHandler::connectToServer() {
        requestConnection();
    }

    void CConnectionHandler::requestConnection() {
        networkHandler->sendPacket(new ConnectionRequestPacket());
    }

    void CConnectionHandler::onConnectionSuccessEvent(ConnectionSuccessfulEvent *event) {
        networkHandler->setUUID(event->getUUID());
        std::cout << "Connected with server. My UUID: " << event->getUUID() << std::endl;
    }

    void CConnectionHandler::onPingEvent(PingEvent *event) {
        networkHandler->sendPacket(new PingPacket());
    }
}
