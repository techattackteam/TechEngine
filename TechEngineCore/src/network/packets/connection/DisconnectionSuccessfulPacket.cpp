#include "DisconnectionSuccessfulPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/DisconnectionSuccessfulEvent.hpp"


namespace TechEngineCore {
    void DisconnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new DisconnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void DisconnectionSuccessfulPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<Packet>(*this);
        ar & uuid;
    }

}

