#include "ConnectionSuccessfulPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"


namespace TechEngineCore {
    void ConnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void ConnectionSuccessfulPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
        ar & uuid;
    }
}

