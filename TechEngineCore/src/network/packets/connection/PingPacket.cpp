#include "PingPacket.hpp"
#include "event/EventDispatcher.hpp"
#include "event/events/connection/PingEvent.hpp"

#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/base_object.hpp>

namespace TechEngineCore {

    PingPacket::PingPacket(const std::string &uuid) : Packet(uuid) {

    }

    void PingPacket::onPacketReceive() {
        EventDispatcher::getInstance().dispatch(new PingEvent(uuid));
    }

    template<class Archive>
    void PingPacket::serialize(Archive &ar, unsigned int version) {
        ar & boost::serialization::base_object<TechEngineCore::Packet>(*this);
    }
}
BOOST_CLASS_EXPORT_IMPLEMENT(TechEngineCore::PingPacket)