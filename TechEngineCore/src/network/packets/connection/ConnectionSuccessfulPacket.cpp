#include "ConnectionSuccessfulPacket.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/connection/ConnectionSuccessfulEvent.hpp"


namespace TechEngine {
    void ConnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void ConnectionSuccessfulPacket::serialize(Archive& ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngine::Packet>(*this);
        ar & uuid;
    }
}
