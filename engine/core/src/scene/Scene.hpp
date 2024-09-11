#pragma once

#include "components/ArchetypesManager.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class CORE_DLL Scene : public System {
    private:
        ArchetypesManager m_archetypesManager;

    public:
        void init() override;

        Entity createEntity(const std::string& name);

        void destroyEntity(Entity entity);

        template<typename T>
        void addComponent(Entity entity, const T& component) {
            m_archetypesManager.addComponent<T>(entity, component);
        }

        template<typename T>
        void removeComponent(Entity entity) {
            m_archetypesManager.removeComponent<T>(entity);
        }

        template<typename T>
        T& getComponent(Entity entity) {
            return m_archetypesManager.getComponent<T>(entity);
        }

        template<typename T>
        bool hasComponent(Entity entity) {
            return m_archetypesManager.hasComponent<T>(entity);
        }

        std::vector<ComponentTypeID> getCommonComponents(const std::vector<Entity>& entities);

        Entity getEntityByTag(const Tag& tag);

        template<typename... Components, typename Function>
        void runSystem(Function function) {
            m_archetypesManager.runSystem<Components...>(function);
        };
    };
}
