#include "ConnectionRequestPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/ConnectionRequestEvent.hpp"


namespace TechEngine {
    void ConnectionRequestPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionRequestEvent(senderEndpoint));
    }

    template<class Archive>
    void ConnectionRequestPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngine::Packet>(*this);
    }
}

