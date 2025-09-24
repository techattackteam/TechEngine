#include "ArchetypesManager.hpp"
#include "Components.hpp"
#include <algorithm>

namespace TechEngine {
    ArchetypesManager::ArchetypesManager() {
        Component::familyCounter = 0;
        registerComponent<Tag>();
        registerComponent<Transform>();
        registerComponent<Camera>();
        registerComponent<MeshRenderer>();
    }


    Archetype* ArchetypesManager::getArchetype(const std::vector<ComponentTypeID>& componentTypes) {
        std::vector<ComponentTypeID> sortedTypes = componentTypes;
        std::sort(sortedTypes.begin(), sortedTypes.end());

        Archetype* archetype = findArchetype(sortedTypes);

        if (!archetype) {
            archetype = createArchetype(sortedTypes);
        }
        return archetype;
    }

    std::vector<Entity> ArchetypesManager::createEntities(Archetype& archetype, size_t count) {
        if (count == 0) {
            return {};
        }
        const size_t archetypeIndex = archetype.m_indexInECS;
        std::vector<Entity> newEntities;
        newEntities.reserve(count);
        archetype.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            Entity newEntity = generateEntityID();
            size_t indexInArchetype = archetype.reserveSlotFor(newEntity);
            m_entityToArchetypeMap[newEntity] = {archetypeIndex, indexInArchetype};
            newEntities.push_back(newEntity);
        }

        return newEntities;
    }


    Entity ArchetypesManager::createEntity() {
        Entity newEntity = generateEntityID();
        return registerEntity(newEntity);
    }

    Entity ArchetypesManager::registerEntity(Entity newEntity) {
        if (m_lastEntityID < newEntity) {
            m_lastEntityID = newEntity;
        }
        Archetype* archetype = findArchetype({});
        if (archetype == nullptr) {
            archetype = createArchetype({});
        }
        size_t indexInArchetype = archetype->reserveSlotFor(newEntity);

        // 4. Store the entity's direct location in our map.
        m_entityToArchetypeMap[newEntity] = {archetype->m_indexInECS, indexInArchetype};
        return newEntity;
    }

    bool ArchetypesManager::removeEntity(Entity entity) {
        auto it = m_entityToArchetypeMap.find(entity);
        if (it == m_entityToArchetypeMap.end()) {
            return false; // Entity doesn't exist.
        }

        const EntityLocation location = it->second;
        Archetype* archetype = m_archetypes.at(location.m_archetypeIndex).get();
        size_t index = location.m_indexInArchetype;
        Entity swappedEntity = archetype->removeEntity(index);
        m_entityToArchetypeMap.erase(entity);

        if (swappedEntity != entity && index < archetype->m_entities.size()) {
            // Update the moved entity's location
            m_entityToArchetypeMap[swappedEntity] = {location.m_archetypeIndex, index};
        }
        return true;
    }


    void ArchetypesManager::clear() {
        m_archetypes.clear();
        m_archetypeLookup.clear();
        m_entityToArchetypeMap.clear();
        m_lastArchetypeID = 0;
        m_lastEntityID = 0;
        //m_componentTypes.clear();
    }

    Archetype* ArchetypesManager::createArchetype(const std::vector<ComponentTypeID>& componentTypes) {
        size_t index = m_archetypes.size();
        std::unique_ptr<Archetype> newArchetype = std::make_unique<Archetype>(generateArchetypeID(), index, componentTypes);

        for (auto type_id: componentTypes) {
            newArchetype->m_componentData[type_id] = m_componentsRegistry.at(type_id).createStorage();
        }
        m_archetypes.push_back(std::move(newArchetype));
        Archetype* newArchetypePtr = m_archetypes.back().get();
        size_t hash = hashComponentTypes(componentTypes);
        m_archetypeLookup[hash] = newArchetypePtr;
        return newArchetypePtr;
    }

    void ArchetypesManager::moveEntityToArchetype(Entity entity, Archetype* sourceArchetype, const ArchetypeEdge& edge) {
        const size_t sourceIndex = m_entityToArchetypeMap.at(entity).m_indexInArchetype;
        Archetype* destinationArchetype = edge.destinationArchetype;

        const size_t destinationIndex = destinationArchetype->reserveSlotFor(entity);

        for (const auto& mapping: edge.componentMappings) {
            mapping.destination_storage->set_from(destinationIndex, *mapping.source_storage, sourceIndex);
        }

        m_entityToArchetypeMap[entity] = {destinationArchetype->m_indexInECS, destinationIndex}; // Assumes m_indexInECS fix is applied

        Entity swappedEntity = sourceArchetype->removeEntity(sourceIndex);
        if (swappedEntity != entity && sourceIndex < sourceArchetype->m_entities.size()) {
            m_entityToArchetypeMap[swappedEntity].m_indexInArchetype = sourceIndex;
        }
    }

    size_t ArchetypesManager::hashComponentTypes(const std::vector<ComponentTypeID>& componentTypes) const {
        size_t seed = componentTypes.size();
        for (auto& i: componentTypes) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

    Archetype* ArchetypesManager::findArchetype(const std::vector<ComponentTypeID>& sortedComponentTypes) const {
        size_t hash = hashComponentTypes(sortedComponentTypes);
        auto it = m_archetypeLookup.find(hash);
        if (it != m_archetypeLookup.end()) {
            return it->second;
        }
        return nullptr;
    }

    ArchetypeEdge ArchetypesManager::buildEdge(Archetype* sourceArchetype, Archetype* destinationArchetype) {
        ArchetypeEdge edge;
        edge.destinationArchetype = destinationArchetype;

        // Iterate through the source archetype's components.
        for (auto& pair: sourceArchetype->m_componentData) {
            ComponentTypeID typeID = pair.first;
            IComponentStorage* sourceStorage = pair.second.get();

            // Check if the destination also has this component.
            // This is the ONLY hash lookup we do, and we do it once per component type.
            auto it = destinationArchetype->m_componentData.find(typeID);
            if (it != destinationArchetype->m_componentData.end()) {
                IComponentStorage* destStorage = it->second.get();
                // If it matches, add the direct storage pointers to our move plan.
                edge.componentMappings.push_back({sourceStorage, destStorage});
            }
        }
        return edge;
    }


    const ArchetypeEdge& ArchetypesManager::findEdgeAdd(Archetype* sourceArchetype, ComponentTypeID componentToAdd) {
        auto it = sourceArchetype->m_addTransition.find(componentToAdd);
        if (it != sourceArchetype->m_addTransition.end()) {
            return it->second;
        }

        std::vector<ComponentTypeID> destTypes = sourceArchetype->getComponentTypes();
        destTypes.push_back(componentToAdd);
        Archetype* destinationArchetype = getArchetype(destTypes);
        ArchetypeEdge newEdge = buildEdge(sourceArchetype, destinationArchetype);
        return sourceArchetype->m_addTransition.emplace(componentToAdd, std::move(newEdge)).first->second;
    }

    const ArchetypeEdge& ArchetypesManager::findEdgeRemove(Archetype* sourceArchetype, ComponentTypeID componentToRemove) {
        auto it = sourceArchetype->m_removeTransition.find(componentToRemove);
        if (it != sourceArchetype->m_removeTransition.end()) {
            return it->second;
        }

        std::vector<ComponentTypeID> destTypes = sourceArchetype->getComponentTypes();
        destTypes.erase(std::remove(destTypes.begin(), destTypes.end(), componentToRemove), destTypes.end());
        Archetype* destinationArchetype = getArchetype(destTypes);

        ArchetypeEdge newEdge = buildEdge(sourceArchetype, destinationArchetype);

        return sourceArchetype->m_removeTransition.emplace(componentToRemove, std::move(newEdge)).first->second;
    }

    uint32_t ArchetypesManager::generateArchetypeID() {
        return ++m_lastArchetypeID;
    }

    Entity ArchetypesManager::generateEntityID() {
        return ++m_lastEntityID;
    }
}
