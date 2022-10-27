#pragma once


#include "network/Packet.hpp"

class PlayerSyncPacket : public TechEngineCore::Packet {

private:
    float y = 0;
    int playerNumber;
public:
    PlayerSyncPacket() = default;

    PlayerSyncPacket(int playerNumber, float y);

    ~PlayerSyncPacket() = default;

    void onPacketReceive() override;

    template<class Archive>
    void serialize(Archive &ar, unsigned int version);
};
