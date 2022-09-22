#pragma once

#include <TechEngineCore.hpp>
#include <boost/serialization/export.hpp>

class PlayerSelectorPacket : public TechEngineCore::Packet {
private:
    int playerNumber = 0;
public:
    PlayerSelectorPacket() = default;

    PlayerSelectorPacket(std::string uuid, int playerNumber);

    ~PlayerSelectorPacket() = default;

    void onPacketReceive();

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);
};

BOOST_CLASS_EXPORT_KEY(PlayerSelectorPacket);