#pragma once

#include "../Packet.hpp"
#include "dataStructure/Queue.hpp"


namespace TechEngineCore {
    class NetworkHandler;

    class PacketHandler {
    protected:
        NetworkHandler *networkHandler;

        Queue incomePackets;
        Queue outcomePackets;

    public:
        PacketHandler(NetworkHandler *networkHandler);

        virtual void init();

        void sendPacket(std::string uuid, Packet *packet, udp::endpoint endpoint);

        bool processIncomePacket();

        void storeIncomePacket(Packet *pPacket);

        Packet *getNextOutcomePacket();
    };
}
