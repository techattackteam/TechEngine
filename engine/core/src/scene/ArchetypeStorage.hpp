#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <scene/Archetype.hpp>
#include <scene/EntitySlots.hpp>

#include <cstddef>
#include <memory>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TechEngine {
    class ArchetypeStorage {
    public:
        using SignatureHasher = std::size_t (*)(std::span<const ComponentDenseId>);

    private:
        ComponentRegistry* m_registry = nullptr;
        EntitySlots m_entities;
        std::vector<std::unique_ptr<Archetype>> m_archetypes;
        std::unordered_map<std::size_t, std::vector<Archetype*>> m_archetypesByHash;
        SignatureHasher m_signatureHasher = nullptr;

    public:
        explicit ArchetypeStorage(ComponentRegistry& registry, SignatureHasher signatureHasher = nullptr);

        Entity createEntity();

        bool destroyEntity(Entity entity);

        bool contains(Entity entity) const;

        void clear();

        template<ComponentValue T>
        bool addComponent(const Entity entity, const T& value) {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || location->archetype->contains(type)) {
                return false;
            }

            Archetype::Edge& edge = addEdge(*location->archetype, type);
            move(entity, *location, edge, [type, &value](Archetype& destination, const std::size_t destinationRow) {
                auto& storage = static_cast<ComponentStorage<T>&>(*destination.m_columns.at(type));
                storage.set(destinationRow, value);
            });
            return true;
        }

        template<ComponentValue T>
        bool removeComponent(const Entity entity) {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || !location->archetype->contains(type)) {
                return false;
            }

            Archetype::Edge& edge = removeEdge(*location->archetype, type);
            move(entity, *location, edge, [](Archetype&, std::size_t) {
            });
            return true;
        }

        template<ComponentValue T>
        T* component(const Entity entity) {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || !location->archetype->contains(type)) {
                return nullptr;
            }
            return &location->archetype->components<T>(type)[location->row];
        }

        template<ComponentValue T>
        const T* component(const Entity entity) const {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || !location->archetype->contains(type)) {
                return nullptr;
            }
            return &location->archetype->components<T>(type)[location->row];
        }

        const EntityLocation* location(Entity entity) const;

        std::size_t archetypeCount() const;

        static std::size_t hashSignature(std::span<const ComponentDenseId> signature);

    private:
        template<ComponentValue T>
        ComponentDenseId denseId() const {
            const ComponentTypeRecord* record = m_registry->find(componentTypeId<T>());
            TE_CHECK(record != nullptr, "Component type is not registered");
            return record->denseId;
        }

        Archetype& getOrCreate(ComponentSignature signature);

        Archetype::Edge buildEdge(Archetype& source, Archetype& destination);

        Archetype::Edge& addEdge(Archetype& source, ComponentDenseId type);

        Archetype::Edge& removeEdge(Archetype& source, ComponentDenseId type);

        template<typename PrepareDestination>
        void move(const Entity entity, const EntityLocation sourceLocation, const Archetype::Edge& edge, PrepareDestination&& prepareDestination) {
            Archetype& source = *sourceLocation.archetype;
            Archetype& destination = *edge.destination;
            const std::size_t destinationRow = destination.append(entity);

            try {
                for (const Archetype::ColumnMapping& column: edge.columns) {
                    column.destination->setCopy(destinationRow, *column.source, sourceLocation.row);
                }
                std::forward<PrepareDestination>(prepareDestination)(destination, destinationRow);
            } catch (...) {
                destination.eraseSwap(destinationRow);
                throw;
            }

            m_entities.setLocation(entity, {&destination, destinationRow});
            const Entity swapped = source.eraseSwap(sourceLocation.row);
            if (swapped != entity) {
                m_entities.setLocation(swapped, {&source, sourceLocation.row});
            }

            TE_CHECK(source.rowCountsMatch() && destination.rowCountsMatch(), "Archetype row counts diverged during a component transition");
        }
    };
}
