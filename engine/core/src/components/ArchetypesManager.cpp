#include "ArchetypesManager.hpp"
#include "Components.hpp"

namespace TechEngine {
    void Archetype::moveEntityComponents(Entity entity, Archetype& newArchetype, ComponentTypeID excludeComponent) {
        auto entityIndex = std::distance(entities.begin(), std::find(entities.begin(), entities.end(), entity));
        for (auto& pair: componentData) {
            ComponentTypeID typeID = pair.first;
            if (typeID == excludeComponent) {
                continue;
            }
            std::vector<char> data = pair.second;
            size_t componentSize = getComponentSize(typeID);
            // Move component data from old to new archetype
            auto start = data.begin() + entityIndex * componentSize;
            auto end = start + componentSize;
            newArchetype.componentData[typeID].insert(newArchetype.componentData[typeID].end(), start, end);
        }
    }

    ArchetypesManager::ArchetypesManager() {
        populateArchetypes();
    }

    void ArchetypesManager::populateArchetypes() {
        //TODO: Implement this function
    }

    std::vector<Archetype> ArchetypesManager::queryArchetypes(const std::vector<ComponentTypeID>& requiredComponents) {
        std::vector<Archetype> matchingArchetypes;
        for (auto& archetype: archetypes) {
            if (archetype.containsComponents(requiredComponents)) {
                matchingArchetypes.push_back(archetype);
            }
        }
        return matchingArchetypes;
    }

    Entity ArchetypesManager::createEntity() {
        Entity newEntity = generateEntityID();
        size_t index = findArchetype({});
        if (index == -1) {
            archetypes.push_back(Archetype(generateArchetypeID()));
            index = archetypes.size() - 1;
        }
        Archetype& newArchetype = archetypes[index];
        newArchetype.addEntity(newEntity);
        entityToArchetypeMap[newEntity] = index;
        return newEntity;
    }

    bool ArchetypesManager::removeEntity(Entity entity) {
        if (entityToArchetypeMap.find(entity) == entityToArchetypeMap.end()) {
            return false;
        }
        size_t index = entityToArchetypeMap[entity];
        Archetype& archetype = archetypes[index];
        archetype.removeEntity(entity);
        return true;
    }

    std::vector<char> ArchetypesManager::getEntityComponents(Entity entity) {
        std::vector<char> components;
        size_t index = entityToArchetypeMap[entity];
        Archetype& archetype = archetypes[index];
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
        size_t index = entityToArchetypeMap[entity];
        Archetype& archetype = archetypes[index];
        for (auto& pair: archetype.componentData) {
            componentTypes.push_back(pair.first);
        }
        return componentTypes;
    }

    size_t ArchetypesManager::findArchetype(const std::vector<ComponentTypeID>& componentTypes) {
        for (size_t i = 0; i < archetypes.size(); ++i) {
            if (archetypes[i].hasMatchingComponents(componentTypes)) {
                return i;
            }
        }
        return -1;
    }

    uint32_t ArchetypesManager::generateArchetypeID() {
        return lastArchetypeID++;
    }

    Entity ArchetypesManager::generateEntityID() {
        return lastEntityID++;
    }
}
