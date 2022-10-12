#include "TechAttack.hpp"
#include "Camera.hpp"
#include <memory>

TechAttack::TechAttack() {
    new Camera();
    networkHandler = new NetworkHandler();
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

}

void TechAttack::onUpdate() {
}

void TechAttack::onFixedUpdate() {

}

void TechAttack::testPacket(TestPacketEvent *event) {
}

void TechAttack::selectPlayer(PlayerSelectorEvent *event) {
}

void TechAttack::syncNetworkPlayer(PlayerSyncEvent *event) {
}

TechEngineCore::App *TechEngineCore::createApp() {
    return new TechAttack();
}

