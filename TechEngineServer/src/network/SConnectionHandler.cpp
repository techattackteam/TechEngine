
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
        TechEngine::EventDispatcher::getInstance().subscribe(TechEngine::ConnectionRequestEvent::eventType, [this](TechEngine::Event *event) {
            onConnectionRequest((TechEngine::ConnectionRequestEvent *) event);
        });

        TechEngine::EventDispatcher::getInstance().subscribe(TechEngine::PingEvent::eventType, [this](TechEngine::Event *event) {
            onPingEvent((TechEngine::PingEvent *) event);
        });

        TechEngine::EventDispatcher::getInstance().subscribe(TechEngine::DisconnectionRequestEvent::eventType, [this](TechEngine::Event *event) {
            onDisconnectionRequest((TechEngine::DisconnectionRequestEvent *) event);
        });

        isAlive = new std::thread(&SConnectionHandler::checkAliveClients, this);
    }

    SConnectionHandler::~SConnectionHandler() {
        isAlive->join();
        delete (isAlive);
    }


    void SConnectionHandler::onConnectionRequest(TechEngine::ConnectionRequestEvent *event) {
        boost::uuids::uuid uuid = uuidGenerator();
        std::string uuidString = boost::uuids::to_string(uuid);
        Client *client = new Client(uuidString, event->getEndpoint());
        networkHandler->getClients().insert(std::make_pair(uuidString, client));
        TechEngine::EventDispatcher::getInstance().dispatch(new TechEngine::ConnectionSuccessfulEvent(uuidString));
        networkHandler->sendPacket(new TechEngine::ConnectionSuccessfulPacket(), client);
        std::cout << "New connection accepted uuid: " << uuidString << std::endl;
    }

    void SConnectionHandler::onDisconnectionRequest(TechEngine::DisconnectionRequestEvent *event) {
        Client *client = networkHandler->getClient(event->getUUID());
        networkHandler->sendPacket(new TechEngine::DisconnectionSuccessfulPacket(), client);
        networkHandler->getClients().erase(event->getUUID());
    }

    void SConnectionHandler::checkAliveClients() {
        do {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::chrono::system_clock::time_point timeStamp = std::chrono::system_clock::now();
            for (const auto &element: ((SNetworkHandler *) networkHandler)->getClients()) {
                Client *client = element.second;
                checkAlive(element.first, timeStamp);
                networkHandler->sendPacket(new TechEngine::PingPacket(), client);
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

    void SConnectionHandler::onPingEvent(TechEngine::PingEvent *event) {
        Client *client = networkHandler->getClient(event->getUUID());
        client->setLastPingTime(std::chrono::system_clock::now());
    }

}

