#pragma once
#include <stdexcept>

#include "Archetype.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    class CORE_DLL ArchetypesManager {
    public:
        std::vector<Archetype> archetypes; // List of archetypes
        std::unordered_map<Entity, size_t> entityToArchetypeMap; // Map of entities to their archetypes
    private:
        ArchetypeID lastArchetypeID = 0;
        Entity lastEntityID = 0;

    public:
        ArchetypesManager();

        void populateArchetypes();

        // Create a new entity with a set of components
        Entity createEntity();

        bool removeEntity(Entity entity);

        template<typename T>
        bool addComponent(Entity entity, const T& component) {
            if (!ComponentType::isComponentRegistered<T>()) {
                ComponentType::init();
                /*TE_LOGGER_CRITICAL("Component type not registered");
                return false;*/
            }
            if (hasComponent<T>(entity)) {
                TE_LOGGER_WARN("Entity already has component of type {0}", typeid(T).name());
                return false;
            }
            /*if (!std::is_trivially_copyable<T>::value) {
                TE_LOGGER_CRITICAL("Component type {0} is not trivial", typeid(T).name());
                return false;
            }*/
            size_t oldArchetypeIndex = entityToArchetypeMap[entity];
            // Collect current component types for the entity's old archetype
            auto componentTypes = archetypes[oldArchetypeIndex].getComponentTypes();
            componentTypes.push_back(ComponentType::get<T>()); // Add the new component type

            size_t newArchetypeIndex = findArchetype(componentTypes);
            if (newArchetypeIndex == -1) {
                archetypes.emplace_back(generateArchetypeID());
                newArchetypeIndex = archetypes.size() - 1;
                archetypes[oldArchetypeIndex] = archetypes[oldArchetypeIndex];
            }

            archetypes[newArchetypeIndex].addEntity(entity);
            archetypes[oldArchetypeIndex].moveEntityComponents(entity, archetypes[newArchetypeIndex]);
            archetypes[newArchetypeIndex].addComponentData<T>(entity, component);
            archetypes[oldArchetypeIndex].removeEntity(entity);
            entityToArchetypeMap[entity] = newArchetypeIndex;
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
            size_t oldArchetypeIndex = entityToArchetypeMap[entity];
            std::vector<ComponentTypeID> componentTypes = archetypes[oldArchetypeIndex].getComponentTypes();
            componentTypes.erase(std::remove(componentTypes.begin(), componentTypes.end(), ComponentType::get<T>()), componentTypes.end());

            size_t newArchetypeIndex = findArchetype(componentTypes);
            if (newArchetypeIndex == -1) {
                archetypes.push_back(Archetype(generateArchetypeID()));
                newArchetypeIndex = archetypes.size() - 1;
            }
            archetypes[newArchetypeIndex].addEntity(entity);
            archetypes[oldArchetypeIndex].moveEntityComponents(entity, archetypes[newArchetypeIndex], ComponentType::get<T>());
            archetypes[oldArchetypeIndex].removeEntity(entity);
            entityToArchetypeMap[entity] = newArchetypeIndex;
            return true;
        }

        template<typename T>
        T& getComponent(Entity entity) {
            if (hasComponent<T>(entity)) {
                size_t index = entityToArchetypeMap[entity];
                Archetype& archetype = archetypes[index];
                return archetype.getComponent<T>(entity);
            } else {
                throw std::runtime_error("Entity does not have component of type " + std::string(typeid(T).name()));
            }
        }

        std::vector<char> getEntityComponents(Entity entity);

        template<typename T>
        bool hasComponent(Entity entity) {
            size_t index = entityToArchetypeMap[entity];
            if (index == -1) {
                return false;
            }
            Archetype& archetype = archetypes[index];
            ComponentTypeID typeID = ComponentType::get<T>();
            return !archetype.componentData.empty() && archetype.componentData.find(typeID) != archetype.componentData.end();
        }

        std::vector<ComponentTypeID> getComponentTypes(Entity entity);

        template<typename... Component, typename Function>
        void runSystem(Function function) {
            for (auto& archetype: archetypes) {
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

    private :
        size_t findArchetype(const std::vector<ComponentTypeID>& componentTypes);


        uint32_t generateArchetypeID();

        Entity generateEntityID();
    };
}
