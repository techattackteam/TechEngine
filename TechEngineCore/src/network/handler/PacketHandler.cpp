#include "PacketHandler.hpp"
#include "NetworkHandler.hpp"

namespace TechEngineCore {
    PacketHandler::PacketHandler(NetworkHandler *networkHandler) : networkHandler(networkHandler), incomePackets(), outcomePackets() {
    }

    void PacketHandler::init() {

    }

    void PacketHandler::sendPacket(Packet *packet, udp::endpoint endpoint) {
        packet->senderEndpoint = endpoint;
        outcomePackets.push(packet);
    }

    bool PacketHandler::processIncomePacket() {
        //Should this be a thread of his own?
        Packet *packet = incomePackets.pop();
        if (packet == nullptr) {
            return false;
        }
        packet->onPacketReceive();
        delete (packet);
        return true;
    }

    void PacketHandler::storeIncomePacket(Packet *packet) {
        incomePackets.push(packet);
    }

    Packet *PacketHandler::getNextOutcomePacket() {
        return outcomePackets.pop();
    }

}