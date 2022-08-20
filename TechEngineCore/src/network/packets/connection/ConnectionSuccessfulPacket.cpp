#include "ConnectionSuccessfulPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/ConnectionSuccessfulEvent.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

namespace TechEngineCore {
    ConnectionSuccessfulPacket::ConnectionSuccessfulPacket(const std::string &uuid) : Packet(uuid) {

    }

    void ConnectionSuccessfulPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new ConnectionSuccessfulEvent(uuid));
    }

    template<class Archive>
    void ConnectionSuccessfulPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
        ar & uuid;
    }

}
BOOST_CLASS_EXPORT_IMPLEMENT(TechEngineCore::ConnectionSuccessfulPacket);

