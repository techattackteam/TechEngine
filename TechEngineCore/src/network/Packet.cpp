#include "Packet.hpp"

namespace TechEngine {

    template<class Archive>
    void Packet::serialize(Archive &ar, unsigned int version) {
        ar & uuid;
    }

    template void Packet::serialize(boost::archive::text_iarchive &arch, const unsigned int version);

    template void Packet::serialize(boost::archive::text_oarchive &arch, const unsigned int version);
}
