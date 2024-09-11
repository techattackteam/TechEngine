#pragma once

#include "core/Logger.hpp"
#include "Archetype.hpp"

namespace TechEngine {
    class CORE_DLL ArchetypesManager {
    public:
        std::vector<Archetype> m_archetypes; // List of archetypes
        std::unordered_map<Entity, size_t> m_entityToArchetypeMap; // Map of entities to their archetypes
        friend class Scene;

    private:
        ArchetypeID m_lastArchetypeID = 0;
        Entity m_lastEntityID = 0;

        ArchetypesManager();

        void populateArchetypes();

        // Create a new entity with a set of components
        Entity createEntity();

        bool removeEntity(Entity entity);

        template<typename T>
        bool addComponent(Entity entity, const T& component) {
            if (!ComponentType::isComponentRegistered<T>()) {
                TE_LOGGER_CRITICAL("Component type not registered");
                return false;
            }
            if (hasComponent<T>(entity)) {
                TE_LOGGER_WARN("Entity already has component of type {0}", typeid(T).name());
                return false;
            }
            /*if (!std::is_trivially_copyable<T>::value) {
                TE_LOGGER_CRITICAL("Component type {0} is not trivial", typeid(T).name());
                return false;
            }*/
            size_t oldArchetypeIndex = m_entityToArchetypeMap[entity];
            // Collect current component types for the entity's old archetype
            auto componentTypes = m_archetypes[oldArchetypeIndex].getComponentTypes();
            componentTypes.push_back(ComponentType::get<T>()); // Add the new component type

            size_t newArchetypeIndex = findArchetype(componentTypes);
            if (newArchetypeIndex == -1) {
                m_archetypes.emplace_back(generateArchetypeID());
                newArchetypeIndex = m_archetypes.size() - 1;
                m_archetypes[oldArchetypeIndex] = m_archetypes[oldArchetypeIndex];
            }

            m_archetypes[newArchetypeIndex].addEntity(entity);
            m_archetypes[oldArchetypeIndex].moveEntityComponents(entity, m_archetypes[newArchetypeIndex]);
            m_archetypes[newArchetypeIndex].addComponentData<T>(entity, component);
            m_archetypes[oldArchetypeIndex].removeEntity(entity);
            m_entityToArchetypeMap[entity] = newArchetypeIndex;
            return true;
        }

        template<typename T>
        bool removeComponent(Entity entity) {
            if (!ComponentType::isComponentRegistered<T>()) {
                //throw std::runtime_error("Component type not registered");
                return false;
            }
            if (!hasComponent<T>(entity)) {
                //throw std::runtime_error("Entity does not have component of type " + std::string(typeid(T).name()));
                return false;
            }
            size_t oldArchetypeIndex = m_entityToArchetypeMap[entity];
            std::vector<ComponentTypeID> componentTypes = m_archetypes[oldArchetypeIndex].getComponentTypes();
            componentTypes.erase(std::remove(componentTypes.begin(), componentTypes.end(), ComponentType::get<T>()), componentTypes.end());

            size_t newArchetypeIndex = findArchetype(componentTypes);
            if (newArchetypeIndex == -1) {
                m_archetypes.push_back(Archetype(generateArchetypeID()));
                newArchetypeIndex = m_archetypes.size() - 1;
            }
            m_archetypes[newArchetypeIndex].addEntity(entity);
            m_archetypes[oldArchetypeIndex].moveEntityComponents(entity, m_archetypes[newArchetypeIndex], ComponentType::get<T>());
            m_archetypes[oldArchetypeIndex].removeEntity(entity);
            m_entityToArchetypeMap[entity] = newArchetypeIndex;
            return true;
        }

        template<typename T>
        T& getComponent(Entity entity) {
            if (hasComponent<T>(entity)) {
                size_t index = m_entityToArchetypeMap[entity];
                Archetype& archetype = m_archetypes[index];
                return archetype.getComponent<T>(entity);
            } else {
                throw std::runtime_error("Entity does not have component of type " + std::string(typeid(T).name()));
            }
        }

        std::vector<char> getEntityComponents(Entity entity);

        template<typename T>
        bool hasComponent(Entity entity) {
            size_t index = m_entityToArchetypeMap[entity];
            if (index == -1) {
                return false;
            }
            Archetype& archetype = m_archetypes[index];
            ComponentTypeID typeID = ComponentType::get<T>();
            return !archetype.componentData.empty() && archetype.componentData.find(typeID) != archetype.componentData.end();
        }

        std::vector<ComponentTypeID> getComponentTypes(Entity entity);

        template<typename... Component, typename Function>
        void runSystem(Function function) {
            for (auto& archetype: m_archetypes) {
                if (!archetype.containsComponents({ComponentType::get<Component>()...})) {
                    continue;
                }
                auto componentArrays = std::make_tuple(&archetype.getComponentArray<Component>()...);
                for (size_t i = 0; i < archetype.entities.size(); i++) {
                    function((*std::get<std::vector<std::decay_t<Component>>*>(componentArrays))[i]...);
                }
            }
        }

        std::vector<Archetype*> queryArchetypes(const std::vector<ComponentTypeID>& requiredComponents);

    private:
        size_t findArchetype(const std::vector<ComponentTypeID>& componentTypes);

        uint32_t generateArchetypeID();

        Entity generateEntityID();
    };
}
