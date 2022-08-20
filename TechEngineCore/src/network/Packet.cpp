#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "Packet.hpp"

namespace TechEngineCore {

    Packet::Packet() = default;

    Packet::Packet(const std::string &uuid) {
        this->uuid = uuid;
    }

    template<class Archive>
    void Packet::serialize(Archive &ar, unsigned int version) {
        ar & uuid;
    }

    template void Packet::serialize(boost::archive::text_iarchive &arch, const unsigned int version);

    template void Packet::serialize(boost::archive::text_oarchive &arch, const unsigned int version);
}
