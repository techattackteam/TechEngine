#pragma once

#include <TechEngineServer.hpp>
#include <EntryPoint.hpp>
#include "src/PlayerSyncEvent.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"

class Server : public TechEngineServer::App {
private:
    bool startGame = false;
    int x = 0;
    int y = 0;
    int direction = 1;

    float player1y = 0;
    std::string player1UUID;
    float player2y = 0;
    std::string player2UUID;
public:
    Server();

    ~Server();

    void onUpdate() override;

    void onFixedUpdate() override;

    void testPacketEvent();

    void SyncBall();

    void onPlayerJoinEvent(TechEngineCore::ConnectionSuccessfulEvent *event);

    void syncPlayers(PlayerSyncEvent *event);
};
