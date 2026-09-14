#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    using ComponentSignature = std::vector<ComponentDenseId>;

    class ArchetypeStorage;

    class Archetype {
    private:
        struct ColumnMapping {
            IComponentStorage* source = nullptr;
            IComponentStorage* destination = nullptr;
        };

        struct Edge {
            Archetype* destination = nullptr;
            std::vector<ColumnMapping> columns;
        };

        friend class ArchetypeStorage;

        ComponentSignature m_signature;
        std::vector<Entity> m_entities;
        std::unordered_map<ComponentDenseId, std::unique_ptr<IComponentStorage>> m_columns;
        std::unordered_map<ComponentDenseId, Edge> m_addTransitions;
        std::unordered_map<ComponentDenseId, Edge> m_removeTransitions;

    public:
        Archetype(ComponentSignature signature, const ComponentRegistry& registry);

        Archetype(const Archetype&) = delete;

        Archetype& operator=(const Archetype&) = delete;

        std::span<const ComponentDenseId> signature() const;

        std::span<const Entity> entities() const;

        bool contains(ComponentDenseId type) const;

        std::size_t rowCount() const;

        bool rowCountsMatch() const;

        template<ComponentValue T>
        std::span<T> components(const ComponentDenseId type) {
            return static_cast<ComponentStorage<T>&>(*m_columns.at(type)).values();
        }

        template<ComponentValue T>
        std::span<const T> components(const ComponentDenseId type) const {
            return static_cast<const ComponentStorage<T>&>(*m_columns.at(type)).values();
        }

    private:
        void reserve(std::size_t capacity);

        std::size_t append(Entity entity);

        Entity eraseSwap(std::size_t row);
    };
}
