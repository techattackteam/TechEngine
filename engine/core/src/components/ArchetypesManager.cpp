#include "ArchetypesManager.hpp"
#include "Components.hpp"

namespace TechEngine {
    ArchetypesManager::ArchetypesManager() {
        TE_LOGGER_INFO("ArchetypesManager created");
        populateArchetypes();
        ComponentType::init();
    }

    void Archetype::moveEntityComponents(Entity entity, Archetype& newArchetype, ComponentTypeID excludeComponent) {
        auto entityIndex = std::distance(entities.begin(), std::find(entities.begin(), entities.end(), entity));
        for (auto& pair: componentData) {
            ComponentTypeID typeID = pair.first;
            if (typeID == excludeComponent) {
                continue;
            }
            std::vector<char> data = pair.second;
            size_t componentSize = getComponentSize(typeID);
            auto start = data.begin() + entityIndex * componentSize;
            auto end = start + componentSize;
            newArchetype.componentData[typeID].insert(newArchetype.componentData[typeID].end(), start, end);
        }
    }


    void ArchetypesManager::populateArchetypes() {

    }


    Entity ArchetypesManager::createEntity() {
        Entity newEntity = generateEntityID();
        size_t index = findArchetype({});
        if (index == -1) {
            m_archetypes.push_back(Archetype(generateArchetypeID()));
            index = m_archetypes.size() - 1;
        }
        Archetype& newArchetype = m_archetypes[index];
        newArchetype.addEntity(newEntity);
        m_entityToArchetypeMap[newEntity] = index;
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
        for (auto& pair: archetype.componentData) {
            ComponentTypeID typeID = pair.first;
            size_t componentSize = archetype.getComponentSize(typeID);
            auto entityIndex = std::distance(archetype.entities.begin(), std::find(archetype.entities.begin(), archetype.entities.end(), entity));
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
        for (auto& pair: archetype.componentData) {
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

    uint32_t ArchetypesManager::generateArchetypeID() {
        return m_lastArchetypeID++;
    }

    Entity ArchetypesManager::generateEntityID() {
        return m_lastEntityID++;
    }
}
