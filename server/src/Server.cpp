#include <iostream>
#include "Server.hpp"
#include "src/TestPacketEvent.hpp"
#include "src/TestPacket.hpp"
#include "src/PlayerSelectorPacket.hpp"
#include "src/PlayerSyncPacket.hpp"
#include "src/PlayerSyncEvent.hpp"


Server::Server() {
    TechEngine::subscribeEvent(TechEngineCore::ConnectionSuccessfulEvent::eventType, [this](TechEngineCore::Event *event) {
        onPlayerJoinEvent((ConnectionSuccessfulEvent *) event);
    });

    TechEngine::subscribeEvent(TestPacketEvent::eventType, [this](TechEngineCore::Event *event) {
        testPacketEvent();
    });

    TechEngine::subscribeEvent(PlayerSyncEvent::eventType, [this](TechEngineCore::Event *event) {
        syncPlayers((PlayerSyncEvent *) event);
    });
}

Server::~Server() {

}

void Server::onUpdate() {
    if (networkHandler.getClients().size() == 2) {
        startGame = true;
    }
}

void Server::onFixedUpdate() {
    if (startGame) {
        if (x > 10) {
            direction = -1;
        } else if (x < -10) {
            direction = 1;
        }

        x += direction;
    }
    SyncBall();
}

void Server::SyncBall() {
    for (auto element: networkHandler.getClients()) {
        networkHandler.getPacketHandler().sendPacket(new TestPacket(element.first, x, y), element.second->endpoint);
    }
}

void Server::testPacketEvent() {
    for (auto element: networkHandler.getClients()) {
        networkHandler.getPacketHandler().sendPacket(new TestPacket(), element.second->endpoint);
    }
}

void Server::onPlayerJoinEvent(ConnectionSuccessfulEvent *event) {
    if (networkHandler.getClients().size() == 1) {
        networkHandler.getPacketHandler().sendPacket(new PlayerSelectorPacket(event->getUUID(), 1), networkHandler.getClient(event->getUUID())->endpoint);
        player1UUID = event->getUUID();
    } else {
        networkHandler.getPacketHandler().sendPacket(new PlayerSelectorPacket(event->getUUID(), 2), networkHandler.getClient(event->getUUID())->endpoint);
        networkHandler.getPacketHandler().sendPacket(new PlayerSyncPacket(event->getUUID(), 1, player1y), networkHandler.getClient(event->getUUID())->endpoint);
        player2UUID = event->getUUID();
    }
}

void Server::syncPlayers(PlayerSyncEvent *event) {
    if (event->playerNumber == 1) {
        player1y = event->y;
        networkHandler.getPacketHandler().sendPacket(new PlayerSyncPacket(player2UUID, 1, player1y), networkHandler.getClient(player2UUID)->endpoint);
    } else {
        player2y = event->y;
        networkHandler.getPacketHandler().sendPacket(new PlayerSyncPacket(player1UUID, 2, player2y), networkHandler.getClient(player1UUID)->endpoint);
    }
}

TechEngineCore::App *TechEngineCore::createApp() {
    return new Server();
}
