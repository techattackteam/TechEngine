#pragma once

#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Query.hpp>

#include <scene/Archetype.hpp>
#include <scene/EntitySlots.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TechEngine {
    class ArchetypeStorage : public internal::QuerySource {
    public:
        using SignatureHasher = std::size_t (*)(std::span<const ComponentDenseId>);

    private:
        ComponentRegistry* m_registry = nullptr;
        EntitySlots m_entities;
        std::vector<std::unique_ptr<Archetype>> m_archetypes;
        std::unordered_map<std::size_t, std::vector<Archetype*>> m_archetypesByHash;
        SignatureHasher m_signatureHasher = nullptr;
        std::uint64_t m_archetypeRevision = 0;
        std::atomic_size_t m_iterationDepth = 0;

    public:
        explicit ArchetypeStorage(ComponentRegistry& registry, SignatureHasher signatureHasher = nullptr);

        ArchetypeStorage(const ArchetypeStorage&) = delete;

        ArchetypeStorage(ArchetypeStorage&&) = delete;

        ArchetypeStorage& operator=(const ArchetypeStorage&) = delete;

        ArchetypeStorage& operator=(ArchetypeStorage&&) = delete;

        Entity createEntity();

        bool destroyEntity(Entity entity);

        bool contains(Entity entity) const;

        void clear();

        template<ComponentValue T>
        bool addComponent(const Entity entity) {
            return addComponent<T>(entity, T());
        }

        template<ComponentValue T>
        bool addComponent(const Entity entity, const T& value) {
            checkStructuralMutationAllowed();
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
            checkStructuralMutationAllowed();
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

        bool addComponent(Entity entity, ComponentDenseId type, const void* value);

        bool removeComponent(Entity entity, ComponentDenseId type);

        template<ComponentValue T>
        T* component(const Entity entity) {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || !location->archetype->contains(type)) {
                return nullptr;
            }
            return &location->archetype->getComponents<T>(type)[location->row];
        }

        template<ComponentValue T>
        const T* component(const Entity entity) const {
            const EntityLocation* location = m_entities.location(entity);
            const ComponentDenseId type = denseId<T>();
            if (location == nullptr || !location->archetype->contains(type)) {
                return nullptr;
            }
            return &location->archetype->getComponents<T>(type)[location->row];
        }

        void* componentRaw(Entity entity, ComponentTypeId type);

        const void* componentRaw(Entity entity, ComponentTypeId type) const;

        const EntityLocation* location(Entity entity) const;

        std::size_t archetypeCount() const;

        void markChanged(ComponentDenseId type, std::uint64_t tick);

        std::uint64_t getChangeTick(Entity entity, ComponentDenseId type) const;

        template<typename Function>
        void eachEntity(Function&& function) {
            beginQueryIteration();
            try {
                for (const std::unique_ptr<Archetype>& archetype: m_archetypes) {
                    for (const Entity entity: archetype->entities()) {
                        std::invoke(function, entity);
                    }
                }
            } catch (...) {
                endQueryIteration();
                throw;
            }
            endQueryIteration();
        }

        template<typename WritableComponents, typename ReadableComponents>
        Query<WritableComponents, ReadableComponents> query() {
            using QueryType = Query<WritableComponents, ReadableComponents>;
            return QueryType(*this);
        }

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

        void checkStructuralMutationAllowed() const;

        std::uint64_t queryRevision() const override;

        std::size_t queryArchetypeCount() const override;

        bool queryArchetypeContains(std::size_t archetypeIndex, ComponentTypeId type) const override;

        const std::vector<Entity>* queryEntities(std::size_t archetypeIndex) const override;

        IComponentStorage* queryColumn(std::size_t archetypeIndex, ComponentTypeId type) override;

        void beginQueryIteration() override;

        void endQueryIteration() override;

        template<typename PrepareDestination>
        void move(const Entity entity, const EntityLocation sourceLocation, const Archetype::Edge& edge, PrepareDestination&& prepareDestination) {
            Archetype& source = *sourceLocation.archetype;
            Archetype& destination = *edge.destination;
            const std::size_t destinationRow = destination.addEntity(entity);

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
