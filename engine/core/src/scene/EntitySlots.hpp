#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace TechEngine {
    class EntitySlots {
    private:
        struct Slot {
            std::uint32_t generation = 0;
            std::uint32_t nextFree = Entity::NULL_INDEX;
            bool occupied = false;
            bool retired = false;
        };

        std::vector<Slot> m_slots;
        std::uint32_t m_freeHead = Entity::NULL_INDEX;
        std::uint32_t m_slotLimit = Entity::NULL_INDEX;
        std::uint32_t m_generationLimit = std::numeric_limits<std::uint32_t>::max();
        std::size_t m_size = 0;

    public:
        explicit EntitySlots(std::uint32_t slotLimit = Entity::NULL_INDEX, std::uint32_t generationLimit = std::numeric_limits<std::uint32_t>::max());

        Entity create();

        bool destroy(Entity entity);

        bool contains(Entity entity) const;

        void clear();

        std::size_t size() const;
    };
}
