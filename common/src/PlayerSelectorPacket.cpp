#include "PlayerSelectorPacket.hpp"
#include "PlayerSelectorEvent.hpp"

PlayerSelectorPacket::PlayerSelectorPacket(int playerNumber) : playerNumber(playerNumber) {

}

void PlayerSelectorPacket::onPacketReceive() {
    TechEngineCore::EventDispatcher::getInstance().dispatch(new PlayerSelectorEvent(playerNumber));
}

template<class Archive>
void PlayerSelectorPacket::serialize(Archive &ar, unsigned int version) {
    ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    ar & playerNumber;
}
