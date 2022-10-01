#pragma once

#include <TechEngine.hpp>
#include <EntryPoint.hpp>
#include "NetworkHandler.hpp"
#include "../../TechEngineEditor/src/testGameObject/QuadMeshTest.hpp"
#include "src/TestPacketEvent.hpp"
#include "src/PlayerSelectorEvent.hpp"
#include "src/PlayerSyncEvent.hpp"

class TechAttack : public TechEngine::App {
public:
    NetworkHandler *networkHandler;

    int playerNumber;

    QuadMeshTest *ball;
    QuadMeshTest *player;
    QuadMeshTest *networkPlayer;

    TechEngineCore::StateMachine stateMachine{};

    TechAttack();

    void onUpdate();

    void keyPressedEvent(TechEngine::KeyHoldEvent *event);

    void onFixedUpdate();

    void sendTestPacket();

    void testPacket(TestPacketEvent *pEvent);

    void selectPlayer(PlayerSelectorEvent *event);

    void syncNetworkPlayer(PlayerSyncEvent *event);
};


