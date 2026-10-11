#pragma once

#include <TechEngine/core/resources/ResourceId.hpp>

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <vector>

namespace TechEngine {
    enum class AssetAvailability : std::uint8_t { Present, AbsentForRole, Unknown };

    class AssetSource {
    public:
        virtual ~AssetSource() = default;

        virtual AssetAvailability find(ResourceId id) const = 0;

        virtual std::error_code read(ResourceId id, std::vector<std::byte>& out) const = 0;
    };
}
