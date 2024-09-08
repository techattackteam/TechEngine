#pragma once
#include <vector>

#include "components/ArchetypesManager.hpp"
#include "components/Components.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class Scene : public System {
    private:
        ArchetypesManager archetypesManager;

    public:
        void init() override;

        Entity createEntity(const std::string& name);

        void destroyEntity(Entity entity);

        std::vector<Archetype> queryArchetypes(const std::vector<ComponentTypeID>& requiredComponents);

        std::vector<Entity> getEntities();

        std::vector<char> getEntityComponents(Entity entity);

        template<typename T>
        void addComponent(Entity entity, const T& component) {
            archetypesManager.addComponent<T>(entity, component);
        }

        template<typename T>
        void removeComponent(Entity entity) {
            archetypesManager.removeComponent<T>(entity);
        }

        template<typename T>
        std::vector<T> getComponents() {
            std::vector<Archetype> archetypes = queryArchetypes({ComponentType::get<T>()});
            std::vector<T> components;
            for (Archetype& archetype: archetypes) {
                std::vector<T> archetypeComponents = archetype.getComponentArray<T>();
                components.insert(components.end(), archetypeComponents.begin(), archetypeComponents.end());
            }
            return components;
        }

        template<typename T>
        T& getComponent(Entity entity) {
            return archetypesManager.getComponent<T>(entity);
        }

        std::vector<ComponentTypeID> getCommonComponents(const std::vector<Entity>& entities);

        template<typename T>
        bool hasComponent(Entity entity) {
            return archetypesManager.hasComponent<T>(entity);
        }

        Entity getEntityByTag(const Tag& tag);
    };
}
