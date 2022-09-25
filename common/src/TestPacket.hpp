#pragma once

#include <TechEngineCore.hpp>
#include "TestPacketEvent.hpp"

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

BOOST_CLASS_EXPORT(TestPacket);