#include "PingPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/PingEvent.hpp"


namespace TechEngineCore {
    void PingPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new PingEvent(uuid));
    }

    template<class Archive>
    void PingPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    }
}