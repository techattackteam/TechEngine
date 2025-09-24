#pragma once

#include "components/ArchetypesManager.hpp"
#include "components/Components.hpp"

namespace TechEngine {
    class CORE_DLL Scene {
    private:
        ArchetypesManager m_archetypesManager;
        std::string m_name = "Untitled";
        friend class SceneSerializer;

    public:
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

        Entity getEntity(const Tag& tag);

        Entity getEntity(const std::string& uuid);

        template<typename... Components, typename Function>
        void runSystem(Function function) {
            m_archetypesManager.query<Components...>().each(function);
        }

        void clear();

        void setName(const std::string& name);

        const std::string& getName() const;

        int getTotalEntities();
    };
}
