#include "DisconnectionSuccessfulPacket.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/connection/DisconnectionSuccessfulEvent.hpp"


namespace TechEngine {
    void DisconnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new DisconnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void DisconnectionSuccessfulPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<Packet>(*this);
        ar & uuid;
    }

}

