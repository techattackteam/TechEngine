#include "PlayerSyncPacket.hpp"
#include "PlayerSyncEvent.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

PlayerSyncPacket::PlayerSyncPacket(std::string uuid, int playerNumber, float y) : playerNumber(playerNumber), y(y), Packet(uuid) {

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

BOOST_CLASS_EXPORT_IMPLEMENT(PlayerSyncPacket);