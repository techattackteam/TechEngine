#include <resources/ResourceSlots.hpp>

namespace TechEngine {
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
        entry = ResourceEntry{};
        entry.generation = generation + 1;
        entry.nextFree = m_freeHead;
        m_freeHead = slot.index;
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
        return m_blocks[index / ENTRIES_PER_BLOCK]->at(index % ENTRIES_PER_BLOCK);
    }

    const ResourceEntry& ResourceSlots::getResourceEntry(const std::uint32_t index) const {
        return m_blocks[index / ENTRIES_PER_BLOCK]->at(index % ENTRIES_PER_BLOCK);
    }
}
