#include <TechEngine/base/diagnostics/Assert.hpp>

#include <scene/EntitySlots.hpp>

namespace TechEngine {
    EntitySlots::EntitySlots(std::uint32_t slotLimit, std::uint32_t generationLimit) : m_slotLimit{slotLimit}, m_generationLimit{generationLimit} {
    }

    Entity EntitySlots::create() {
        if (m_freeHead != Entity::NULL_INDEX) {
            Slot& slot = m_slots[m_freeHead];
            Entity entity{m_freeHead, slot.generation};
            slot.occupied = true;
            m_freeHead = slot.nextFree;
            slot.nextFree = Entity::NULL_INDEX;
            m_size++;
            return entity;
        }
        TE_CHECK(m_slots.size() < m_slotLimit, "Entity slot limit exceeded after {0} entities", m_slotLimit);
        const std::uint32_t index = static_cast<std::uint32_t>(m_slots.size());
        m_slots.push_back({0, Entity::NULL_INDEX, true});
        m_size++;
        return Entity{index, 0};
    }

    bool EntitySlots::destroy(Entity entity) {
        if (entity.index < m_slots.size() && m_slots[entity.index].occupied && m_slots[entity.index].generation == entity.generation) {
            Slot& slot = m_slots[entity.index];
            slot.occupied = false;

            if (slot.generation == m_generationLimit) {
                slot.retired = true;
                slot.nextFree = Entity::NULL_INDEX;
            } else {
                slot.generation++;
                slot.nextFree = m_freeHead;
                m_freeHead = entity.index;
            }

            m_size--;
            return true;
        }
        return false;
    }

    bool EntitySlots::contains(Entity entity) const {
        return entity.index < m_slots.size() && m_slots[entity.index].occupied && m_slots[entity.index].generation == entity.generation;
    }

    void EntitySlots::clear() {
        m_freeHead = Entity::NULL_INDEX;

        for (std::size_t index = 0; index < m_slots.size(); index++) {
            Slot& slot = m_slots[index];

            if (slot.occupied) {
                slot.occupied = false;

                if (slot.generation == m_generationLimit) {
                    slot.retired = true;
                } else {
                    slot.generation++;
                }
            }

            slot.nextFree = Entity::NULL_INDEX;

            if (!slot.retired) {
                slot.nextFree = m_freeHead;
                m_freeHead = static_cast<std::uint32_t>(index);
            }
        }

        m_size = 0;
    }

    std::size_t EntitySlots::size() const {
        return m_size;
    }
}
