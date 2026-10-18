#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/ResourceHandle.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

namespace TechEngine {
    using ResourcePayloadDeleter = void (*)(void*);

    struct ResourceEntry {
        ResourceId id;
        StringId typeTag;
        std::unique_ptr<void, ResourcePayloadDeleter> payload{nullptr, nullptr};
        std::vector<ResourceSlotId> dependencies;
        std::uint32_t acquires = 0;
        std::uint32_t generation = 0;
        std::uint32_t nextFree = ResourceSlotId::NULL_INDEX;
        ResourceState state = ResourceState::Pending;
        bool occupied = false;
    };

    class ResourceSlots {
    public:
        static constexpr std::uint32_t ENTRIES_PER_BLOCK = 256;

    private:
        // The vector of block pointers reallocates, the blocks themselves never do. An entry
        // pointer stays valid for the life of its block, so never hand a caller a block
        // pointer or an index into m_blocks.
        std::vector<std::unique_ptr<std::array<ResourceEntry, ENTRIES_PER_BLOCK>>> m_blocks;
        std::uint32_t m_freeHead = ResourceSlotId::NULL_INDEX;
        std::uint32_t m_slotCount = 0;
        std::uint32_t m_slotLimit = ResourceSlotId::NULL_INDEX;
        std::uint32_t m_generationLimit = std::numeric_limits<std::uint32_t>::max();
        std::size_t m_liveCount = 0;

    public:
        explicit ResourceSlots(std::uint32_t slotLimit = ResourceSlotId::NULL_INDEX, std::uint32_t generationLimit = std::numeric_limits<std::uint32_t>::max());

        ResourceSlotId create();

        bool destroy(ResourceSlotId slot);

        ResourceEntry* find(ResourceSlotId slot);

        const ResourceEntry* find(ResourceSlotId slot) const;

        std::size_t size() const;

    private:
        bool isSlotValid(ResourceSlotId slot) const;

        ResourceEntry& getResourceEntry(std::uint32_t index);

        const ResourceEntry& getResourceEntry(std::uint32_t index) const;
    };
}
