#include "DisconnectionSuccessfulPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/DisconnectionSuccessfulEvent.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

namespace TechEngineCore {
    DisconnectionSuccessfulPacket::DisconnectionSuccessfulPacket(const std::string &uuid) : Packet(uuid) {

    }

    void DisconnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new DisconnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void DisconnectionSuccessfulPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<Packet>(*this);
        ar & uuid;
    }

}
BOOST_CLASS_EXPORT_IMPLEMENT(TechEngineCore::DisconnectionSuccessfulPacket);

