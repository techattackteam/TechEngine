#pragma once

#include "Components.hpp"

namespace TechEngine {
    using ArchetypeID = uint32_t;

    class CORE_DLL Archetype {
    private:
        friend class ArchetypesManager;
        friend class SceneSerializer;
        ArchetypeID m_id;
        std::vector<Entity> m_entities; // List of entities in this archetype
        std::unordered_map<ComponentTypeID, std::vector<char>> m_componentData; // Storage for component data

    public:
        explicit Archetype(ArchetypeID id) : m_id(id) {
        }

        //Copy constructor
        Archetype(const Archetype& archetype) {
            m_id = archetype.m_id;
            m_entities = archetype.m_entities;
            m_componentData = archetype.m_componentData;
        }

        template<typename T>
        T& getComponent(Entity entity) {
            ComponentTypeID typeID = ComponentType::get<T>();
            auto& dataVector = m_componentData[typeID];
            size_t index = std::distance(m_entities.begin(), std::find(m_entities.begin(), m_entities.end(), entity));
            size_t componentSize = sizeof(T);
            char& component = dataVector[index * componentSize];
            return *reinterpret_cast<T*>(&component);
        }

        // get a reference to the component array for a specific type
        template<typename T>
        std::vector<T>& getComponentArray() {
            ComponentTypeID typeID = ComponentType::get<T>();
            auto& dataVector = m_componentData[typeID];
            return *reinterpret_cast<std::vector<T>*>(&dataVector);
        }

        bool operator==(const Archetype& lhr) const {
            if (m_id != lhr.m_id) {
                return false;
            } else {
                return true;
            }
        }

        // Method to get the component types for this archetype
        std::vector<ComponentTypeID> getComponentTypes() const;

        // Method to get the entities in this archetype
        std::vector<Entity> getEntities() const;

        void clear();

    private:
        void addEntity(Entity entity);

        void removeEntity(Entity entity);

        void moveEntityComponents(Entity entity, Archetype& newArchetype, ComponentTypeID excludeComponent = -1);

        // Add component data for a specific component type
        template<typename T>
        void addComponentData(Entity entity, const T& component) {
            ComponentTypeID typeID = ComponentType::get<T>();
            size_t entityIndex = std::distance(m_entities.begin(), std::find(m_entities.begin(), m_entities.end(), entity));
            auto& componentVector = m_componentData[typeID];

            // Ensure there's enough space to store the new component
            if (componentVector.size() < (entityIndex + 1) * sizeof(T)) {
                componentVector.resize((entityIndex + 1) * sizeof(T));
            }

            // Copy the component data to the correct position
            std::memcpy(&componentVector[entityIndex * sizeof(T)], &component, sizeof(T));
        }

        // Remove component data for a specific component type
        template<typename T>
        void removeComponentData(Entity entity) {
            ComponentTypeID typeID = ComponentType::get<T>();
            auto& componentVector = m_componentData[typeID];

            auto entityIndex = std::distance(m_entities.begin(), std::find(m_entities.begin(), m_entities.end(), entity));
            size_t componentSize = sizeof(T);

            // Erase the component data for this entity
            if (entityIndex == m_entities.size() - 1) {
                componentVector.erase(componentVector.begin() + entityIndex * componentSize, componentVector.end());
            } else {
                componentVector.erase(componentVector.begin() + entityIndex * componentSize, componentVector.begin() + (entityIndex + 1) * componentSize);
            }
        }

        size_t getComponentSize(ComponentTypeID typeID);

        bool hasMatchingComponents(const std::vector<ComponentTypeID>& requiredComponents);

        bool containsComponents(const std::vector<ComponentTypeID>& componentTypes);
    };
}
