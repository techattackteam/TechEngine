#pragma once

#include <TechEngineCore.hpp>

class PlayerSyncEvent : public TechEngineCore::Event {
public:
    float y;
    int playerNumber;
    static inline EventType eventType = EventType("PlayerSyncEvent", ASYNC);

    PlayerSyncEvent(int playerNumber, float y) : playerNumber(playerNumber), y(y), Event(eventType) {
    };

    ~PlayerSyncEvent() override = default;

};
