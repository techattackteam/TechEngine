#pragma once

#include "CoreExportDLL.hpp"

#include <string>
#include <unordered_set>
#include <shared_mutex>

namespace TechEngine {
    class CORE_DLL UUID {
    private:
        uint64_t uuid = -1;
        inline static std::unordered_set<uint64_t> registeredUUIDs;
        inline static std::shared_mutex mutex;

        struct UnlockedTag {};

        static uint64_t generateUniqueUUIDLocked();

        UUID(uint64_t uuid, UnlockedTag) noexcept;

    public:
        UUID();

        UUID(uint64_t uuid);

        UUID(const UUID& other) = default;

        UUID(UUID&& other) noexcept = default;

        UUID& operator=(const UUID& other) = default;

        UUID& operator=(UUID&& other) noexcept = default;

        ~UUID() = default;

        explicit operator uint64_t() const {
            return uuid;
        }

        bool operator==(const UUID& other) const {
            return uuid == other.uuid;
        }

        static UUID generate();

        static void registerUUID(uint64_t existingUUID) {
            std::lock_guard lock(mutex);
            registeredUUIDs.insert(existingUUID);
        }

        [[nodiscard]] bool isNull() const;

        [[nodiscard]] std::string toString() const;
    };
}

namespace std {
    template<>
    struct hash<TechEngine::UUID> {
        std::size_t operator()(const TechEngine::UUID& uuid) const noexcept {
            return static_cast<std::size_t>(uuid);
        }
    };
}
