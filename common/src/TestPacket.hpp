#pragma once

#include "TestPacketEvent.hpp"
#include "network/Packet.hpp"

class TestPacket : public TechEngineCore::Packet {
private:
    int x = 0;
    int y = 0;
public:
    TestPacket() = default;

    TestPacket(int x, int y);

    ~TestPacket() = default;

    void onPacketReceive();

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);
};
