#pragma once
#include "core/CoreExportDll.hpp"
#include <cstdint>
#include <string>
#include <unordered_set>
#include <xhash>

namespace TechEngine {
    class CORE_DLL UUID {
    private:
        uint64_t uuid;
        inline static std::unordered_set<uint64_t> registeredUUIDs;

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

        static UUID generate();

        static void registerUUID(uint64_t existingUUID) {
            registeredUUIDs.insert(existingUUID);
        }

        std::string toString() const;
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
