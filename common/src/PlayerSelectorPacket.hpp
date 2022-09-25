#pragma once

#include <TechEngineCore.hpp>
#include "PlayerSelectorEvent.hpp"

class PlayerSelectorPacket : public TechEngineCore::Packet {
public:
    int playerNumber = 0;

    PlayerSelectorPacket() = default;

    PlayerSelectorPacket(int playerNumber);

    ~PlayerSelectorPacket() override = default;

    void onPacketReceive() override;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);
};

BOOST_CLASS_EXPORT(PlayerSelectorPacket)