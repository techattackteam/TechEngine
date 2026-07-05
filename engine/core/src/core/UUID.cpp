#include "TechEngine/core/core/UUID.hpp"

#include <random>
#include <shared_mutex>


namespace TechEngine {
    static std::random_device s_RandomDevice;
    static std::mt19937_64 s_Engine(s_RandomDevice());
    static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

    uint64_t UUID::generateUniqueUUIDLocked() {
        uint64_t newUUID;
        do {
            newUUID = s_UniformDistribution(s_Engine);
        } while (registeredUUIDs.find(newUUID) != registeredUUIDs.end());
        registeredUUIDs.insert(newUUID);
        return newUUID;
    }

    UUID::UUID(uint64_t uuid, UnlockedTag) noexcept : uuid(uuid) {
    }

    UUID::UUID() {
        std::lock_guard lock(mutex);
        uuid = generateUniqueUUIDLocked();
    }

    UUID::UUID(uint64_t uuid) : uuid(uuid) {
        std::lock_guard lock(mutex);
        registeredUUIDs.insert(uuid);
    }

    UUID UUID::generate() {
        std::lock_guard lock(mutex);
        return UUID(generateUniqueUUIDLocked(), UnlockedTag{});
    }

    bool UUID::isNull() const {
        return uuid == -1;
    }

    std::string UUID::toString() const {
        return std::to_string(uuid);
    }
}
