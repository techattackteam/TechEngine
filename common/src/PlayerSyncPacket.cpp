#include "PlayerSyncPacket.hpp"
#include "PlayerSyncEvent.hpp"

PlayerSyncPacket::PlayerSyncPacket(int playerNumber, float y) : playerNumber(playerNumber), y(y) {

}

void PlayerSyncPacket::onPacketReceive() {
    TechEngineCore::EventDispatcher::getInstance().dispatch(new PlayerSyncEvent(playerNumber, y));
}

template<class Archive>
void PlayerSyncPacket::serialize(Archive &ar, unsigned int version) {
    ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    ar & y;
    ar & playerNumber;
}
