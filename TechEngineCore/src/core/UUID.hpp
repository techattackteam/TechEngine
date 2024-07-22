#pragma once
#include "core/CoreExportDll.hpp"
#include <cstdint>
#include <string>
#include <xhash>

namespace TechEngine {
    class CORE_DLL UUID {
    private:
        uint64_t uuid;

    public:
        UUID();

        UUID(uint64_t uuid);

        UUID(const UUID& other) = default;

        UUID(UUID&& other) noexcept = default;

        UUID& operator=(const UUID& other) = default;

        UUID& operator=(UUID&& other) noexcept = default;

        ~UUID() = default;

        operator uint64_t() const {
            return uuid;
        }

        static UUID generate() {
            return UUID();
        }
    };
}

namespace std {
    template<>
    struct hash<TechEngine::UUID> {
        std::size_t operator()(const TechEngine::UUID& uuid) const noexcept {
            return uuid;
        }
    };
}
