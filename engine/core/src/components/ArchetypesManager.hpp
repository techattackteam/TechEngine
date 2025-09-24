#pragma once
#include "core/CoreExportDLL.hpp"
#include "Archetype.hpp"
#include "Query.hpp"

namespace TechEngine {
    class CORE_DLL ArchetypesManager {
    public:
        struct EntityLocation {
            size_t m_archetypeIndex;
            size_t m_indexInArchetype;
        };

        std::vector<ComponentTypeID> m_componentTypes; // List of registered component types
        std::unordered_map<ComponentTypeID, ComponentInfo> m_componentsRegistry; // List of component types

        std::vector<std::unique_ptr<Archetype>> m_archetypes; // List of archetypes
        std::unordered_map<Entity, EntityLocation> m_entityToArchetypeMap; // Map of entities to their archetypes

    private:
        std::unordered_map<size_t, Archetype*> m_archetypeLookup; // For quick archetype finding

        ArchetypeID m_lastArchetypeID = 0;
        Entity m_lastEntityID = 0;

    public:
        ArchetypesManager();

        ~ArchetypesManager() = default;

        ArchetypesManager(const ArchetypesManager&) = delete;

        ArchetypesManager& operator=(const ArchetypesManager&) = delete;

        template<typename T>
        bool registerComponent() {
            ComponentTypeID typeID = ComponentType<T>::get();
            if (std::find(m_componentTypes.begin(), m_componentTypes.end(), typeID) == m_componentTypes.end()) {
                m_componentTypes.push_back(typeID);
                m_componentsRegistry[typeID] = {
                    sizeof(T),
                    alignof(T),
                    []() {
                        return std::make_unique<ComponentStorage<T>>();
                    }
                };
                return true;
            }
            return false;
        }

        Archetype* getArchetype(const std::vector<ComponentTypeID>& componentTypes);

        std::vector<Entity> createEntities(Archetype& archetype, size_t count);

        Entity createEntity();

        Entity registerEntity(Entity newEntity);

        bool removeEntity(Entity entity);

        template<typename T>
        bool addComponent(Entity entity, const T& component) {
            auto it = m_entityToArchetypeMap.find(entity);
            if (it == m_entityToArchetypeMap.end() || hasComponent<T>(entity)) {
                return false;
            }

            const EntityLocation& sourceLocation = it->second;
            Archetype* sourceArchetype = m_archetypes.at(sourceLocation.m_archetypeIndex).get();

            const ArchetypeEdge& edge = findEdgeAdd(sourceArchetype, ComponentType<T>::get());

            // Centralize the move logic.
            moveEntityToArchetype(entity, sourceArchetype, edge);

            // After the move, the entity is in its new slot. Now we just need to
            // set the data for the component we just added.
            const auto newLocation = m_entityToArchetypeMap.at(entity);
            edge.destinationArchetype->setComponentData<T>(newLocation.m_indexInArchetype, component);

            return true;
        }

        template<typename T>
        bool removeComponent(Entity entity) {
            auto it = m_entityToArchetypeMap.find(entity);
            if (it == m_entityToArchetypeMap.end() || !hasComponent<T>(entity)) {
                return false;
            }

            const EntityLocation sourceLocation = it->second;
            Archetype* sourceArchetype = m_archetypes.at(sourceLocation.m_archetypeIndex).get();

            const ArchetypeEdge& edge = findEdgeRemove(sourceArchetype, ComponentType<T>::get());

            // Centralize the move logic. No extra data needs to be set.
            moveEntityToArchetype(entity, sourceArchetype, edge);


            return true;
        }

        template<typename T>
        T& getComponent(Entity entity) {
            const EntityLocation location = m_entityToArchetypeMap.at(entity);
            Archetype* archetype = m_archetypes.at(location.m_archetypeIndex).get();
            return archetype->getComponent<T>(location.m_indexInArchetype);
        }

        template<typename T>
        bool hasComponent(Entity entity) {
            if (m_entityToArchetypeMap.contains(entity)) {
                const EntityLocation location = m_entityToArchetypeMap.at(entity);
                Archetype* archetype = m_archetypes.at(location.m_archetypeIndex).get();
                return archetype->containsComponents({ComponentType<T>::get()});
            }
            return false;
        }

        template<typename... Components>
        Query<Components...> query() {
            Query<Components...> newQuery(this);
            std::vector<ComponentTypeID> requiredTypes;
            (void)std::initializer_list<int>{
                (
                    (void)[&] {
                        if constexpr (!std::is_same_v<Components, Entity>) {
                            requiredTypes.push_back(ComponentType<Components>::get());
                        }
                    }(), 0)...
            };
            for (auto& archetype_ptr: m_archetypes) {
                if (archetype_ptr->containsComponents(requiredTypes)) {
                    newQuery.m_matchingArchetypes.push_back(archetype_ptr.get());
                }
            }
            return newQuery;
        }

        void clear();

    private:
        Archetype* createArchetype(const std::vector<ComponentTypeID>& componentTypes);

        void moveEntityToArchetype(Entity entity, Archetype* sourceArchetype, const ArchetypeEdge& edge);

        // Note: The types vector MUST be sorted before calling this function
        // to ensure that {Pos, Vel} and {Vel, Pos} produce the same hash.
        size_t hashComponentTypes(const std::vector<ComponentTypeID>& componentTypes) const;

        Archetype* findArchetype(const std::vector<ComponentTypeID>& componentTypes) const;

        ArchetypeEdge buildEdge(Archetype* sourceArchetype, Archetype* destinationArchetype);

        const ArchetypeEdge& findEdgeAdd(Archetype* sourceArchetype, ComponentTypeID componentToAdd);

        const ArchetypeEdge& findEdgeRemove(Archetype* sourceArchetype, ComponentTypeID componentToRemove);

        uint32_t generateArchetypeID();

        Entity generateEntityID();
    };
}
