#include "UUID.hpp"

#include <random>


namespace TechEngine {
    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    UUID::UUID() : uuid(s_UniformDistribution(s_Engine)) {
        registeredUUIDs.insert(uuid);
    }

    UUID::UUID(uint64_t uuid) : uuid(uuid) {
        registeredUUIDs.insert(uuid);
    }

    UUID UUID::generate() {
        uint64_t newUUID;
        do {
            newUUID = s_UniformDistribution(s_Engine);
        } while (registeredUUIDs.find(newUUID) != registeredUUIDs.end());
        registeredUUIDs.insert(newUUID);
        return UUID(newUUID);
    }

    std::string UUID::toString() const {
        return std::to_string(uuid);
    }
}
