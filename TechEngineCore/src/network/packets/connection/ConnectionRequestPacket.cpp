#include "ConnectionRequestPacket.hpp"
#include "../../../event/EventDispatcher.hpp"
#include "../../../event/events/connection/ConnectionRequestEvent.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

namespace TechEngineCore {
    void ConnectionRequestPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionRequestEvent(senderEndpoint));
    }

    template<class Archive>
    void ConnectionRequestPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    }
}
BOOST_CLASS_EXPORT_IMPLEMENT(TechEngineCore::ConnectionRequestPacket);

