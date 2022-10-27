#include <iostream>
#include "Server.hpp"
#include "src/TestPacketEvent.hpp"
#include "src/TestPacket.hpp"
#include "src/PlayerSelectorPacket.hpp"
#include "src/PlayerSyncPacket.hpp"
#include "src/PlayerSyncEvent.hpp"
#include "wrapper/Wrapper.hpp"


Server::Server() {
    TechEngine::subscribeEvent(TechEngineCore::ConnectionSuccessfulEvent::eventType, [this](TechEngineCore::Event *event) {
        onPlayerJoinEvent((TechEngineCore::ConnectionSuccessfulEvent *) event);
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
        networkHandler.sendPacket(new TestPacket(x, y), element.second);
    }
}

void Server::testPacketEvent() {
    for (auto element: networkHandler.getClients()) {
        networkHandler.sendPacket(new TestPacket(), element.second);
    }
}

void Server::onPlayerJoinEvent(TechEngineCore::ConnectionSuccessfulEvent *event) {
    if (networkHandler.getClients().size() == 1) {
        networkHandler.sendPacket(new PlayerSelectorPacket(1), event->getUUID());
        player1UUID = event->getUUID();
    } else {
        networkHandler.sendPacket(new PlayerSelectorPacket(2), event->getUUID());
        networkHandler.sendPacket(new PlayerSyncPacket(1, player1y), event->getUUID());
        player2UUID = event->getUUID();
    }
}

void Server::syncPlayers(PlayerSyncEvent *event) {
    if (event->playerNumber == 1) {
        player1y = event->y;
        networkHandler.sendPacket(new PlayerSyncPacket(1, player1y), player2UUID);
    } else {
        player2y = event->y;
        networkHandler.sendPacket(new PlayerSyncPacket(2, player2y), player1UUID);
    }
}

TechEngineCore::App *TechEngineCore::createApp() {
    return new Server();
}
