#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <scene/ArchetypeStorage.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    ArchetypeStorage::ArchetypeStorage(ComponentRegistry& registry, SignatureHasher signatureHasher) : m_registry(&registry), m_signatureHasher(signatureHasher) {
        if (m_signatureHasher == nullptr) {
            m_signatureHasher = &hashSignature;
        }
    }

    Entity ArchetypeStorage::createEntity() {
        checkStructuralMutationAllowed();
        Archetype& archetype = getOrCreate({denseId<Hierarchy>(), denseId<Transform>()});
        Entity entity = m_entities.create();
        std::size_t row = 0;
        try {
            row = archetype.addEntity(entity);
        } catch (...) {
            m_entities.destroy(entity);
            throw;
        }
        m_entities.setLocation(entity, {&archetype, row});
        return entity;
    }

    bool ArchetypeStorage::destroyEntity(const Entity entity) {
        checkStructuralMutationAllowed();
        const EntityLocation* location = m_entities.location(entity);
        if (location == nullptr) {
            return false;
        }

        Archetype* archetype = location->archetype;
        const std::size_t row = location->row;
        const Entity swapped = archetype->eraseSwap(row);
        if (swapped != entity) {
            m_entities.setLocation(swapped, {archetype, row});
        }
        return m_entities.destroy(entity);
    }

    bool ArchetypeStorage::addComponent(const Entity entity, const ComponentDenseId type, const void* value) {
        checkStructuralMutationAllowed();
        const EntityLocation* location = m_entities.location(entity);
        if (location == nullptr || location->archetype->contains(type)) {
            return false;
        }

        TE_CHECK(m_registry->find(type) != nullptr, "Component type is not registered");
        TE_CHECK(value != nullptr, "Component value is null");
        Archetype::Edge& edge = addEdge(*location->archetype, type);
        move(entity, *location, edge, [type, value](Archetype& destination, const std::size_t destinationRow) {
            destination.m_columns.at(type)->setCopyFromRaw(destinationRow, value);
        });
        return true;
    }

    bool ArchetypeStorage::removeComponent(const Entity entity, const ComponentDenseId type) {
        checkStructuralMutationAllowed();
        const EntityLocation* location = m_entities.location(entity);
        if (location == nullptr || !location->archetype->contains(type)) {
            return false;
        }

        Archetype::Edge& edge = removeEdge(*location->archetype, type);
        move(entity, *location, edge, [](Archetype&, std::size_t) {
        });
        return true;
    }

    bool ArchetypeStorage::contains(const Entity entity) const {
        return m_entities.contains(entity);
    }

    void* ArchetypeStorage::componentRaw(const Entity entity, const ComponentTypeId type) {
        const EntityLocation* location = m_entities.location(entity);
        const ComponentTypeRecord* record = m_registry->find(type);
        if (location == nullptr || record == nullptr) {
            return nullptr;
        }

        const auto column = location->archetype->m_columns.find(record->denseId);
        if (column == location->archetype->m_columns.end()) {
            return nullptr;
        }
        return column->second->element(location->row);
    }

    const void* ArchetypeStorage::componentRaw(const Entity entity, const ComponentTypeId type) const {
        const EntityLocation* location = m_entities.location(entity);
        const ComponentTypeRecord* record = m_registry->find(type);
        if (location == nullptr || record == nullptr) {
            return nullptr;
        }

        const auto column = location->archetype->m_columns.find(record->denseId);
        if (column == location->archetype->m_columns.end()) {
            return nullptr;
        }
        const IComponentStorage& storage = *column->second;
        return storage.element(location->row);
    }

    void ArchetypeStorage::clear() {
        checkStructuralMutationAllowed();
        m_entities.clear();
        m_archetypesByHash.clear();
        m_archetypes.clear();
        m_archetypeRevision++;
    }

    const EntityLocation* ArchetypeStorage::location(const Entity entity) const {
        return m_entities.location(entity);
    }

    std::size_t ArchetypeStorage::archetypeCount() const {
        return m_archetypes.size();
    }

    void ArchetypeStorage::markChanged(const ComponentDenseId type, const std::uint64_t tick) {
        TE_PROFILER_FUNCTION();
        for (const std::unique_ptr<Archetype>& archetype: m_archetypes) {
            const auto column = archetype->m_columns.find(type);
            if (column != archetype->m_columns.end()) {
                column->second->markChanged(tick);
            }
        }
    }

    std::uint64_t ArchetypeStorage::getChangeTick(const Entity entity, const ComponentDenseId type) const {
        const EntityLocation* location = m_entities.location(entity);
        if (location == nullptr) {
            return 0;
        }

        const auto column = location->archetype->m_columns.find(type);
        if (column == location->archetype->m_columns.end()) {
            return 0;
        }

        return column->second->getChangeTick();
    }

    std::uint64_t ArchetypeStorage::queryRevision() const {
        return m_archetypeRevision;
    }

    std::size_t ArchetypeStorage::queryArchetypeCount() const {
        return m_archetypes.size();
    }

    bool ArchetypeStorage::queryArchetypeContains(const std::size_t archetypeIndex, const ComponentTypeId type) const {
        return m_archetypes[archetypeIndex]->contains(m_registry->denseId(type));
    }

    const std::vector<Entity>* ArchetypeStorage::queryEntities(const std::size_t archetypeIndex) const {
        return &m_archetypes[archetypeIndex]->m_entities;
    }

    IComponentStorage* ArchetypeStorage::queryColumn(const std::size_t archetypeIndex, const ComponentTypeId type) {
        return m_archetypes[archetypeIndex]->m_columns.at(m_registry->denseId(type)).get();
    }

    std::size_t ArchetypeStorage::hashSignature(const std::span<const ComponentDenseId> signature) {
        std::size_t seed = signature.size();
        for (const ComponentDenseId type: signature) {
            seed ^= static_cast<std::size_t>(type.value()) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
        }
        return seed;
    }

    Archetype& ArchetypeStorage::getOrCreate(ComponentSignature signature) {
        std::ranges::sort(signature);
        const auto duplicate = std::ranges::adjacent_find(signature);
        TE_CHECK(duplicate == signature.end(), "Archetype signature contains duplicate component type {0}", duplicate->value());

        const std::size_t hash = m_signatureHasher(signature);
        auto& bucket = m_archetypesByHash[hash];
        const auto existing = std::ranges::find_if(bucket, [&signature](const Archetype* archetype) {
            return std::ranges::equal(archetype->signature(), signature);
        });
        if (existing != bucket.end()) {
            return **existing;
        }

        auto archetype = std::make_unique<Archetype>(std::move(signature), *m_registry);
        Archetype* result = archetype.get();
        m_archetypes.push_back(std::move(archetype));
        bucket.push_back(result);
        m_archetypeRevision++;
        return *result;
    }

    Archetype::Edge ArchetypeStorage::buildEdge(Archetype& source, Archetype& destination) {
        Archetype::Edge edge;
        edge.destination = &destination;
        for (auto& [type, sourceColumn]: source.m_columns) {
            const auto destinationColumn = destination.m_columns.find(type);
            if (destinationColumn != destination.m_columns.end()) {
                edge.columns.push_back({sourceColumn.get(), destinationColumn->second.get()});
            }
        }
        return edge;
    }

    Archetype::Edge& ArchetypeStorage::addEdge(Archetype& source, const ComponentDenseId type) {
        const auto cached = source.m_addTransitions.find(type);
        if (cached != source.m_addTransitions.end()) {
            return cached->second;
        }

        ComponentSignature destinationSignature(source.signature().begin(), source.signature().end());
        destinationSignature.push_back(type);
        Archetype& destination = getOrCreate(std::move(destinationSignature));
        return source.m_addTransitions.emplace(type, buildEdge(source, destination)).first->second;
    }

    Archetype::Edge& ArchetypeStorage::removeEdge(Archetype& source, const ComponentDenseId type) {
        const auto cached = source.m_removeTransitions.find(type);
        if (cached != source.m_removeTransitions.end()) {
            return cached->second;
        }

        ComponentSignature destinationSignature(source.signature().begin(), source.signature().end());
        std::erase(destinationSignature, type);
        Archetype& destination = getOrCreate(std::move(destinationSignature));
        return source.m_removeTransitions.emplace(type, buildEdge(source, destination)).first->second;
    }

    void ArchetypeStorage::checkStructuralMutationAllowed() const {
        TE_CHECK(m_iterationDepth.load(std::memory_order_relaxed) == 0, "Structural mutation is prohibited during query iteration");
    }

    void ArchetypeStorage::beginQueryIteration() {
        m_iterationDepth.fetch_add(1, std::memory_order_relaxed);
    }

    void ArchetypeStorage::endQueryIteration() {
        const std::size_t previousDepth = m_iterationDepth.fetch_sub(1, std::memory_order_relaxed);
        TE_CHECK(previousDepth > 0, "Query iteration depth underflow");
    }

}
