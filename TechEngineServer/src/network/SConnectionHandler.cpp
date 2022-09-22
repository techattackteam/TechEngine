
#include <iostream>
#include "SConnectionHandler.hpp"
#include "SNetworkHandler.hpp"
#include "network/packets/connection/ConnectionSuccessfulPacket.hpp"
#include "network/packets/connection/PingPacket.hpp"
#include "event/events/connection/DisconnectionRequestEvent.hpp"
#include "network/packets/connection/DisconnectionSuccessfulPacket.hpp"

namespace TechEngineServer {
    SConnectionHandler::SConnectionHandler(SNetworkHandler *networkHandler) : ConnectionHandler(networkHandler) {
        this->networkHandler = networkHandler;
        EventDispatcher::getInstance().subscribe(ConnectionRequestEvent::eventType, [this](Event *event) {
            onConnectionRequest((ConnectionRequestEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(PingEvent::eventType, [this](Event *event) {
            onPingEvent((PingEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(DisconnectionRequestEvent::eventType, [this](Event *event) {
            onDisconnectionRequest((DisconnectionRequestEvent *) event);
        });

        isAlive = new std::thread(&SConnectionHandler::checkAliveClients, this);
    }

    SConnectionHandler::~SConnectionHandler() {
        isAlive->join();
        delete (isAlive);
    }


    void SConnectionHandler::onConnectionRequest(ConnectionRequestEvent *event) {
        boost::uuids::uuid uuid = uuidGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        Client *client = new Client(uuidString, event->getEndpoint());
        networkHandler->getClients().insert(std::make_pair(uuidString, client));
        EventDispatcher::getInstance().dispatch(new ConnectionSuccessfulEvent(uuidString));
        networkHandler->getPacketHandler().sendPacket(new ConnectionSuccessfulPacket(client->UUID), event->getEndpoint());
        std::cout << "New connection accepted uuid: " << uuidString << std::endl;
    }

    void SConnectionHandler::onDisconnectionRequest(DisconnectionRequestEvent *event) {
        Client *client = networkHandler->getClient(event->getUUID());
        networkHandler->getPacketHandler().sendPacket(new DisconnectionSuccessfulPacket(client->UUID), client->endpoint);
        networkHandler->getClients().erase(event->getUUID());
    }

    void SConnectionHandler::checkAliveClients() {
        do {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::chrono::system_clock::time_point timeStamp = std::chrono::system_clock::now();
            for (const auto &element: ((SNetworkHandler *) networkHandler)->getClients()) {
                Client *client = element.second;
                checkAlive(element.first, timeStamp);
                networkHandler->getPacketHandler().sendPacket(new PingPacket(element.first), client->endpoint);
            }
        } while (networkHandler->isRunning());
    }

    void SConnectionHandler::checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp) {
        Client *client = ((SNetworkHandler *) networkHandler)->getClient(uuid);
        std::chrono::duration<double> deltaTime = timeStamp - client->getLastPingTime();
        if (deltaTime.count() > 5) {
            //timeoutClient(client);
        }
    }

    void SConnectionHandler::timeoutClient(Client *client) {
        networkHandler->getClients().erase(client->UUID);
    }

    void SConnectionHandler::onPingEvent(PingEvent *event) {
        Client *client = networkHandler->getClient(event->getUUID());
        client->setLastPingTime(std::chrono::system_clock::now());
    }

}

