#include "PingPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/PingEvent.hpp"


namespace TechEngine {
    void PingPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new PingEvent(uuid));
    }

    template<class Archive>
    void PingPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngine::Packet>(*this);
    }
}