#pragma once

#include "TechEngine/core/components/ArchetypesManager.hpp"
#include "TechEngine/core/components/Components.hpp"

namespace TechEngine {
    class SystemsRegistry;

    class CORE_DLL Scene {
    private:
        class Internal;
        friend class Internal;
        friend class SceneSerializer;
        friend class ScenesManager;

        std::unique_ptr<Internal> m_internal;

        ArchetypesManager m_archetypesManager;

        SystemsRegistry& m_systemsRegistry;

        Scene(SystemsRegistry& systemsRegistry);

    public :
        ~Scene();

        Entity createEntity(const std::string& name);

        Entity duplicateEntity(Entity sourceEntity);

        void destroyEntity(Entity entity);

        template<typename T>
        void addComponent(Entity entity, const T& component) {
            m_archetypesManager.addComponent<T>(entity, component);
            addComponentInternal(entity, ComponentType<T>::get());
        }

        template<typename T>
        void removeComponent(Entity entity) {
            m_archetypesManager.removeComponent<T>(entity);
            removeComponentInternal(entity, ComponentType<T>::get());
        }

        template<typename T>
        T& getComponent(Entity entity) {
            return m_archetypesManager.getComponent<T>(entity);
        }

        template<typename T>
        bool hasComponent(Entity entity) {
            return m_archetypesManager.hasComponent<T>(entity);
        }

        Entity getEntity(const Tag& tag);

        Entity getEntity(const std::string& uuid);

        template<typename... Components, typename Function>
        void runSystem(Function function) {
            m_archetypesManager.query<Components...>().each(function);
        }

        template<typename... Components, typename Function>
        void runParallelSystem(Function function) {
            m_archetypesManager.query<Components...>().parallelEach(function);
        }

        void setParent(Entity child, Entity parent);

        void setParentPreservingWorldTransform(Entity parent, Entity child);

        void unParent(Entity child);

        std::vector<Entity> getChildren(Entity entity);

        void clear();

        const std::string& getName() const;

        std::unique_ptr<Internal>& getInternal();

    private:
        void addComponentInternal(Entity entity, ComponentTypeID componentTypeID);

        void removeComponentInternal(Entity entity, ComponentTypeID componentTypeID);

        Entity duplicateEntityRecursive(Entity sourceEntity, Entity newParent);
    };
}
