#pragma once

#include <TechEngineCore.hpp>
#include <boost/serialization/export.hpp>

class PlayerSyncPacket : public TechEngineCore::Packet {

private:
    float y = 0;
    int playerNumber;
public:
    PlayerSyncPacket() = default;

    PlayerSyncPacket(std::string uuid, int playerNumber, float y);

    ~PlayerSyncPacket() = default;

    void onPacketReceive();

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);

};

BOOST_CLASS_EXPORT_KEY(PlayerSyncPacket);