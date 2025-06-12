#include "ArchetypesManager.hpp"
#include "Components.hpp"

namespace TechEngine {
    ArchetypesManager::ArchetypesManager() {
        populateArchetypes();
        ComponentType::init();
    }

    void Archetype::moveEntityComponents(Entity entity, Archetype& newArchetype, ComponentTypeID excludeComponent) {
        auto entityIndex = std::distance(m_entities.begin(), std::find(m_entities.begin(), m_entities.end(), entity));
        for (auto& pair: m_componentData) {
            ComponentTypeID typeID = pair.first;
            if (typeID == excludeComponent) {
                continue;
            }
            std::vector<char> data = pair.second;
            size_t componentSize = getComponentSize(typeID);
            auto start = data.begin() + entityIndex * componentSize;
            auto end = start + componentSize;
            newArchetype.m_componentData[typeID].insert(newArchetype.m_componentData[typeID].end(), start, end);
        }
    }


    void ArchetypesManager::populateArchetypes() {
    }

    Entity ArchetypesManager::registerEntity(const Entity entity) {
        size_t index = findArchetype({});
        if (index == -1) {
            m_archetypes.emplace_back(generateArchetypeID());
            index = m_archetypes.size() - 1;
        }
        if (m_lastEntityID <= entity) {
            m_lastEntityID = entity;
            m_lastEntityID++;
        }
        Archetype& newArchetype = m_archetypes[index];
        newArchetype.addEntity(entity);
        m_entityToArchetypeMap[entity] = index;
        return entity;
    }


    Entity ArchetypesManager::createEntity() {
        Entity newEntity = generateEntityID();
        registerEntity(newEntity);
        return newEntity;
    }

    bool ArchetypesManager::removeEntity(Entity entity) {
        if (m_entityToArchetypeMap.find(entity) == m_entityToArchetypeMap.end()) {
            return false;
        }
        size_t index = m_entityToArchetypeMap[entity];
        Archetype& archetype = m_archetypes[index];
        archetype.removeEntity(entity);
        return true;
    }

    std::vector<char> ArchetypesManager::getEntityComponents(Entity entity) {
        std::vector<char> components;
        size_t index = m_entityToArchetypeMap[entity];
        Archetype& archetype = m_archetypes[index];
        for (auto& pair: archetype.m_componentData) {
            ComponentTypeID typeID = pair.first;
            size_t componentSize = archetype.getComponentSize(typeID);
            auto entityIndex = std::distance(archetype.m_entities.begin(),
                                             std::find(archetype.m_entities.begin(), archetype.m_entities.end(),
                                                       entity));
            auto start = pair.second.begin() + entityIndex * componentSize;
            auto end = start + componentSize;
            components.insert(components.end(), start, end);
        }
        return components;
    }

    std::vector<ComponentTypeID> ArchetypesManager::getComponentTypes(Entity entity) {
        std::vector<ComponentTypeID> componentTypes;
        size_t index = m_entityToArchetypeMap[entity];
        Archetype& archetype = m_archetypes[index];
        for (auto& pair: archetype.m_componentData) {
            componentTypes.push_back(pair.first);
        }
        return componentTypes;
    }

    size_t ArchetypesManager::findArchetype(const std::vector<ComponentTypeID>& componentTypes) {
        for (size_t i = 0; i < m_archetypes.size(); ++i) {
            if (m_archetypes[i].hasMatchingComponents(componentTypes)) {
                return i;
            }
        }
        return -1;
    }

    std::vector<Archetype*> ArchetypesManager::queryArchetypes(const std::vector<ComponentTypeID>& requiredComponents) {
        std::vector<Archetype*> matchingArchetypes;
        for (auto& archetype: m_archetypes) {
            if (archetype.containsComponents(requiredComponents)) {
                matchingArchetypes.push_back(&archetype);
            }
        }
        return matchingArchetypes;
    }

    void ArchetypesManager::clear() {
        for (Archetype& archetype: m_archetypes) {
            archetype.clear();
        }
        m_archetypes.clear();
        m_lastArchetypeID = 0;
        m_lastEntityID = 0;
    }

    uint32_t ArchetypesManager::generateArchetypeID() {
        return m_lastArchetypeID++;
    }

    Entity ArchetypesManager::generateEntityID() {
        TE_LOGGER_INFO("Generating entity ID: {}", m_lastEntityID);
        return m_lastEntityID++;
    }
}
