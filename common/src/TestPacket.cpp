#include "TestPacket.hpp"
#include "TestPacketEvent.hpp"
#include "event/EventDispatcher.hpp"

TestPacket::TestPacket(int x, int y) {
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