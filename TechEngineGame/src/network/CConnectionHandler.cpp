#include <iostream>
#include "CConnectionHandler.hpp"
#include "network/packets/connection/PingPacket.hpp"
#include "network/packets/connection/ConnectionRequestPacket.hpp"

namespace TechEngine {
    CConnectionHandler::CConnectionHandler(CNetworkHandler *networkHandler) : networkHandler(networkHandler), ConnectionHandler(networkHandler) {
        TechEngineCore::EventDispatcher::getInstance().subscribe(TechEngineCore::PingEvent::eventType, [this](TechEngineCore::Event *event) {
            onPingEvent((TechEngineCore::PingEvent *) event);
        });

        TechEngineCore::EventDispatcher::getInstance().subscribe(TechEngineCore::ConnectionSuccessfulEvent::eventType, [this](TechEngineCore::Event *event) {
            onConnectionSuccessEvent((TechEngineCore::ConnectionSuccessfulEvent *) event);
        });
    }

    CConnectionHandler::~CConnectionHandler() {
    }

    void CConnectionHandler::connectToServer() {
        requestConnection();
    }

    void CConnectionHandler::requestConnection() {
        networkHandler->sendPacket(new TechEngineCore::ConnectionRequestPacket());
    }

    void CConnectionHandler::onConnectionSuccessEvent(TechEngineCore::ConnectionSuccessfulEvent *event) {
        networkHandler->setUUID(event->getUUID());
        std::cout << "Connected with server. My UUID: " << event->getUUID() << std::endl;
    }

    void CConnectionHandler::onPingEvent(TechEngineCore::PingEvent *event) {
        networkHandler->sendPacket(new TechEngineCore::PingPacket());
    }
}
