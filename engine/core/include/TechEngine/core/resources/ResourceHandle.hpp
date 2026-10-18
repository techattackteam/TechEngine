#pragma once

#include <compare>
#include <cstdint>
#include <limits>

namespace TechEngine {
    struct ResourceSlotId {
        static constexpr std::uint32_t NULL_INDEX = std::numeric_limits<std::uint32_t>::max();

        std::uint32_t index = NULL_INDEX;
        std::uint32_t generation = 0;

        constexpr bool valid() const {
            return index != NULL_INDEX;
        }

        bool operator==(const ResourceSlotId&) const = default;
        auto operator<=>(const ResourceSlotId&) const = default;
    };

    template<typename T>
    struct ResourceHandle {
        std::uint32_t index = ResourceSlotId::NULL_INDEX;
        std::uint32_t generation = 0;

        constexpr bool valid() const {
            return index != ResourceSlotId::NULL_INDEX;
        }

        constexpr ResourceSlotId slot() const {
            return ResourceSlotId{index, generation};
        }

        bool operator==(const ResourceHandle&) const = default;
        auto operator<=>(const ResourceHandle&) const = default;
    };
}
