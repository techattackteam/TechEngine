#pragma once

#include <TechEngineCore.hpp>
#include <boost/serialization/export.hpp>

class TestPacket : public TechEngineCore::Packet {
private:
    int x = 0;
    int y = 0;
public:
    TestPacket() = default;

    TestPacket(std::string uuid, int x, int y);

    ~TestPacket() = default;

    void onPacketReceive() override;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);

};

BOOST_CLASS_EXPORT_KEY(TestPacket);