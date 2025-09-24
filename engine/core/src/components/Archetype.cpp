#include "Archetype.hpp"
#include <cassert>

namespace TechEngine {
    std::vector<ComponentTypeID> Archetype::getComponentTypes() const {
        return m_componentTypes;
    }

    const std::vector<Entity>& Archetype::getEntities() const {
        return m_entities;
    }

    void Archetype::reserve(size_t count) {
        if (count == 0) return;

        // Calculate the total required capacity.
        size_t new_capacity = m_entities.size() + count;

        // 1. Reserve space for the entity IDs.
        m_entities.reserve(new_capacity);

        // 2. Delegate the reserve call to every component storage.
        for (auto& pair: m_componentData) {
            pair.second->reserve(new_capacity);
        }
    }


    void Archetype::addEntity(Entity entity) {
        if (std::find(m_entities.begin(), m_entities.end(), entity) == m_entities.end()) {
            m_entities.push_back(entity);
        }
    }


    Entity Archetype::removeEntity(size_t index) {
        assert(index < m_entities.size() && "Index out of bounds");

        Entity swappedEntity = m_entities.back();
        size_t entityCount = m_entities.size(); // The size BEFORE we pop.

        if (index < entityCount - 1) {
            m_entities[index] = swappedEntity;
        }

        m_entities.pop_back();
        for (auto& pair: m_componentData) {
            pair.second->remove(index, entityCount);
        }

        return swappedEntity;
    }

    size_t Archetype::getComponentSize(ComponentTypeID typeID) {
        if (m_componentData.find(typeID) != m_componentData.end()) {
            return m_componentData[typeID]->size() / m_entities.size();
        } else {
            return 0;
        }
    }

    bool Archetype::containsComponents(const std::vector<ComponentTypeID>& componentTypes) {
        for (ComponentTypeID typeID: componentTypes) {
            if (m_componentData.find(typeID) == m_componentData.end()) {
                return false;
            }
        }
        return true;
    }

    size_t Archetype::reserveSlotFor(Entity entity) {
        size_t newIndex = m_entities.size();
        m_entities.push_back(entity);
        // Grow ALL component arrays by one default element, ensuring they stay in sync.
        for (auto& pair: m_componentData) {
            pair.second->push_back_default();
        }
        return newIndex;
    }
}
