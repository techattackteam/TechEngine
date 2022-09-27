#include "TechAttack.hpp"
#include "Camera.hpp"
#include "NewObjectPanel.hpp"
#include "src/PlayerSyncPacket.hpp"
#include <memory>

TechAttack::TechAttack() {
    new Camera();
    ball = new QuadMeshTest(-1);
    networkHandler = new NetworkHandler();
    player = new QuadMeshTest(1);
    networkPlayer = new QuadMeshTest(2);
    TechEngine::subscribeEvent(TechEngine::KeyHoldEvent::eventType, [this](TechEngineCore::Event *event) {
        keyPressedEvent((TechEngine::KeyHoldEvent *) event);
    });

    TechEngine::subscribeEvent(PlayerSelectorEvent::eventType, [this](TechEngineCore::Event *event) {
        selectPlayer((PlayerSelectorEvent *) event);
    });

    TechEngine::subscribeEvent(TestPacketEvent::eventType, [this](TechEngineCore::Event *event) {
        testPacket((TestPacketEvent *) event);
    });

    TechEngine::subscribeEvent(PlayerSyncEvent::eventType, [this](TechEngineCore::Event *event) {
        syncNetworkPlayer((PlayerSyncEvent *) event);
    });
}

void TechAttack::keyPressedEvent(TechEngine::KeyHoldEvent *event) {
    if (event->getKey().getKeyCode() == KeyCode::W && player->getTransform().position.y <= 10) {
        player->getTransform().position.y += 0.2;
        networkHandler->getComponent<TechEngine::NetworkHandlerComponent>()->sendPacket(
                new PlayerSyncPacket(playerNumber, player->getTransform().position.y));
    } else if (event->getKey().getKeyCode() == KeyCode::S && player->getTransform().position.y >= -10) {
        player->getTransform().position.y -= 0.2;
        networkHandler->getComponent<TechEngine::NetworkHandlerComponent>()->sendPacket(
                new PlayerSyncPacket(playerNumber, player->getTransform().position.y));
    }
}

void TechAttack::onUpdate() {
}

void TechAttack::onFixedUpdate() {

}

void TechAttack::testPacket(TestPacketEvent *event) {
    ball->getTransform().position.x = event->x;
    ball->getTransform().position.y = event->y;
}

void TechAttack::selectPlayer(PlayerSelectorEvent *event) {
    playerNumber = event->playerNumber;
    if (event->playerNumber == 1) {
        player->getTransform().position.x = -8;
    } else {
        player->getTransform().position.x = 8;
    }
}

void TechAttack::syncNetworkPlayer(PlayerSyncEvent *event) {
    networkPlayer->getTransform().position.y = event->y;
    networkPlayer->getTransform().position.x = playerNumber == 1 ? 8 : -8;
}

TechEngineCore::App *TechEngineCore::createApp() {
    return new TechAttack();
}

