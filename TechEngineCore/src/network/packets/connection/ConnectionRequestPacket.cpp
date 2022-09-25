#include "ConnectionRequestPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/ConnectionRequestEvent.hpp"


namespace TechEngineCore {
    void ConnectionRequestPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionRequestEvent(senderEndpoint));
    }

    template<class Archive>
    void ConnectionRequestPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    }
}

