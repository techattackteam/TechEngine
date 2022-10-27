#pragma once

#include "PlayerSelectorEvent.hpp"
#include "network/Packet.hpp"

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