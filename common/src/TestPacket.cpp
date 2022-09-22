#include "TestPacket.hpp"
#include "TestPacketEvent.hpp"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

TestPacket::TestPacket(std::string uuid, int x, int y) : TechEngineCore::Packet(uuid) {
    this->x = x;
    this->y = y;
}

void TestPacket::onPacketReceive() {
    TechEngineCore::EventDispatcher::getInstance().dispatch(new TestPacketEvent(x, y));
}

template<class Archive>
void TestPacket::serialize(Archive &ar, unsigned int version) {
    ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    ar & x;
    ar & y;
}

BOOST_CLASS_EXPORT_IMPLEMENT(TestPacket);
