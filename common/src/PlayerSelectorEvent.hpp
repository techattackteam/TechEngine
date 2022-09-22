#pragma once

#include <TechEngineCore.hpp>

class PlayerSelectorEvent : public TechEngineCore::Event {
public:
    int playerNumber;
    static inline EventType eventType = EventType("PlayerSelectorEvent", ASYNC);

    PlayerSelectorEvent(int playerNumber) : playerNumber(playerNumber), Event(eventType) {
    };

    ~PlayerSelectorEvent() override = default;

};
