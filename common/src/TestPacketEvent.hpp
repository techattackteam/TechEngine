#pragma once

#include <TechEngineCore.hpp>

class TestPacketEvent : public TechEngineCore::Event {
public:
    int x;
    int y;
    static inline EventType eventType = EventType("TestPacketEvent", ASYNC);

    TestPacketEvent(int x, int y) : x(x), y(y), Event(eventType) {
    };

    ~TestPacketEvent() override = default;

};
