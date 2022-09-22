#include "PlayerSelectorPacket.hpp"
#include "PlayerSelectorEvent.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

PlayerSelectorPacket::PlayerSelectorPacket(std::string uuid, int playerNumber) : playerNumber(playerNumber), Packet(uuid) {

}

void PlayerSelectorPacket::onPacketReceive() {
    TechEngineCore::EventDispatcher::getInstance().dispatch(new PlayerSelectorEvent(playerNumber));
}

template<class Archive>
void PlayerSelectorPacket::serialize(Archive &ar, unsigned int version) {
    ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    ar & playerNumber;
}

BOOST_CLASS_EXPORT_IMPLEMENT(PlayerSelectorPacket);