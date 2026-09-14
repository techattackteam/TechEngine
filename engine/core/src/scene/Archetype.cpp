#include <TechEngine/base/diagnostics/Assert.hpp>

#include <scene/Archetype.hpp>

#include <algorithm>
#include <utility>

namespace TechEngine {
    Archetype::Archetype(ComponentSignature signature, const ComponentRegistry& registry) : m_signature(std::move(signature)) {
        for (const ComponentDenseId type: m_signature) {
            const ComponentTypeRecord* record = registry.find(type);
            TE_CHECK(record != nullptr, "Archetype signature contains an unregistered component type: {0}", type.value());
            m_columns.emplace(type, record->createStorage());
        }
    }

    std::span<const ComponentDenseId> Archetype::signature() const {
        return m_signature;
    }

    std::span<const Entity> Archetype::entities() const {
        return m_entities;
    }

    bool Archetype::contains(const ComponentDenseId type) const {
        return std::binary_search(m_signature.begin(), m_signature.end(), type);
    }

    std::size_t Archetype::rowCount() const {
        return m_entities.size();
    }

    bool Archetype::rowCountsMatch() const {
        return std::ranges::all_of(m_columns, [this](const auto& column) {
            return column.second->size() == m_entities.size();
        });
    }

    void Archetype::reserve(const std::size_t capacity) {
        m_entities.reserve(capacity);
        for (auto& entry: m_columns) {
            entry.second->reserve(capacity);
        }
    }

    std::size_t Archetype::append(const Entity entity) {
        const std::size_t row = m_entities.size();
        m_entities.push_back(entity);
        try {
            for (auto& entry: m_columns) {
                entry.second->appendDefault();
            }
        } catch (...) {
            for (auto& entry: m_columns) {
                if (entry.second->size() > row) {
                    entry.second->popBack();
                }
            }
            m_entities.pop_back();
            throw;
        }
        TE_CHECK(rowCountsMatch(), "Archetype columns do not match the entity row count after append");
        return row;
    }

    Entity Archetype::eraseSwap(const std::size_t row) {
        TE_CHECK(row < m_entities.size(), "Archetype row is out of bounds: {0}", row);
        const Entity swapped = m_entities.back();
        if (row + 1U < m_entities.size()) {
            m_entities[row] = swapped;
        }
        m_entities.pop_back();
        for (auto& entry: m_columns) {
            entry.second->eraseSwap(row);
        }
        TE_CHECK(rowCountsMatch(), "Archetype columns do not match the entity row count after removal");
        return swapped;
    }
}
