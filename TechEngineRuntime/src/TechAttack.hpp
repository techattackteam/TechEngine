#pragma once

#include <EntryPoint.hpp>
#include "NetworkHandler.hpp"
#include "src/TestPacketEvent.hpp"
#include "src/PlayerSelectorEvent.hpp"
#include "src/PlayerSyncEvent.hpp"
#include "core/Window.hpp"
#include "events/input/KeyHoldEvent.hpp"

class TechAttack : public TechEngine::App {
public:
    NetworkHandler *networkHandler;
    TechEngine::Window window{"TechAttack", 1200, 600};
    int playerNumber;

    TechEngineCore::StateMachine stateMachine{};

    TechAttack();

    void onUpdate() override;

    void keyPressedEvent(TechEngine::KeyHoldEvent *event);

    void onFixedUpdate();

    void sendTestPacket();

    void testPacket(TestPacketEvent *pEvent);

    void selectPlayer(PlayerSelectorEvent *event);

    void syncNetworkPlayer(PlayerSyncEvent *event);
};


