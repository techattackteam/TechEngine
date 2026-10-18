#include <TechEngine/base/diagnostics/Assert.hpp>

#include <resources/ResourceSlots.hpp>

namespace TechEngine {
    ResourceSlots::ResourceSlots(const std::uint32_t slotLimit, const std::uint32_t generationLimit) : m_slotLimit(slotLimit), m_generationLimit(generationLimit) {
    }

    ResourceSlotId ResourceSlots::create() {
        if (m_freeHead != ResourceSlotId::NULL_INDEX) {
            const std::uint32_t index = m_freeHead;
            ResourceEntry& reused = getResourceEntry(index);
            m_freeHead = reused.nextFree;
            reused.nextFree = ResourceSlotId::NULL_INDEX;
            reused.occupied = true;
            m_liveCount++;
            return ResourceSlotId{index, reused.generation};
        }

        TE_CHECK(m_slotCount < m_slotLimit, "Resource slot limit exceeded after {0} slots", m_slotLimit);
        if (m_slotCount % ENTRIES_PER_BLOCK == 0) {
            m_blocks.push_back(std::make_unique<std::array<ResourceEntry, ENTRIES_PER_BLOCK>>());
        }
        const std::uint32_t index = m_slotCount;
        m_slotCount++;
        ResourceEntry& entry = getResourceEntry(index);
        entry.occupied = true;
        m_liveCount++;
        return ResourceSlotId{index, entry.generation};
    }

    bool ResourceSlots::destroy(const ResourceSlotId slot) {
        if (!isSlotValid(slot)) {
            return false;
        }

        ResourceEntry& entry = getResourceEntry(slot.index);
        const std::uint32_t generation = entry.generation;
        // A slot whose generation can no longer advance is retired instead of recycled: reusing
        // it would let a handle from its last life resolve against the next entry.
        const bool retiring = generation == m_generationLimit;
        entry = ResourceEntry{};
        entry.generation = retiring ? generation : generation + 1;
        if (!retiring) {
            entry.nextFree = m_freeHead;
            m_freeHead = slot.index;
        }
        m_liveCount--;
        return true;
    }

    ResourceEntry* ResourceSlots::find(const ResourceSlotId slot) {
        if (!isSlotValid(slot)) {
            return nullptr;
        }
        return &getResourceEntry(slot.index);
    }

    const ResourceEntry* ResourceSlots::find(const ResourceSlotId slot) const {
        if (!isSlotValid(slot)) {
            return nullptr;
        }
        return &getResourceEntry(slot.index);
    }

    std::size_t ResourceSlots::size() const {
        return m_liveCount;
    }

    bool ResourceSlots::isSlotValid(const ResourceSlotId slot) const {
        if (slot.index >= m_slotCount) {
            return false;
        }
        const ResourceEntry& entry = getResourceEntry(slot.index);
        return entry.occupied && entry.generation == slot.generation;
    }

    ResourceEntry& ResourceSlots::getResourceEntry(const std::uint32_t index) {
        TE_ASSERT(index < m_slotCount, "Resource slot index {0} is past the {1} slots that exist", index, m_slotCount);
        return (*m_blocks[index / ENTRIES_PER_BLOCK])[index % ENTRIES_PER_BLOCK];
    }

    const ResourceEntry& ResourceSlots::getResourceEntry(const std::uint32_t index) const {
        TE_ASSERT(index < m_slotCount, "Resource slot index {0} is past the {1} slots that exist", index, m_slotCount);
        return (*m_blocks[index / ENTRIES_PER_BLOCK])[index % ENTRIES_PER_BLOCK];
    }
}
