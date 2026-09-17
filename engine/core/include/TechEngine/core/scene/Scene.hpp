#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace TechEngine {
    class ArchetypeStorage;
    class ComponentRegistry;
    class Hierarchy;

    class Scene {
    private:
        std::unique_ptr<ArchetypeStorage> m_storage;

    public:
        explicit Scene(ComponentRegistry& registry);

        ~Scene();

        Scene(const Scene&) = delete;

        Scene(Scene&&) = delete;

        Scene& operator=(const Scene&) = delete;

        Scene& operator=(Scene&&) = delete;

        Entity createEntity() const;

        bool destroyEntity(Entity entity);

        bool contains(Entity entity) const;

        void clear();

        Entity getParent(Entity entity) const;

        std::vector<Entity> getRoots();

        std::vector<Entity> getChildren(Entity parent) const;

        bool setParent(Entity child, Entity parent, std::size_t position = 0);

        bool unparent(Entity child);

        bool reorderChild(Entity child, std::size_t position);

    private:
        Hierarchy* getHierarchy(Entity entity);

        const Hierarchy* getHierarchy(Entity entity) const;
    };
}
