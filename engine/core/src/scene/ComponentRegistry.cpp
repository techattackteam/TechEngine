#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>

namespace TechEngine {
    ComponentRegistry::ComponentRegistry(std::uint32_t typeLimit) : m_typeLimit(typeLimit) {
        TE_CHECK(typeLimit <= ComponentDenseId::VALUE_COUNT, "Component type limit exceeds the dense id capacity: {0}", typeLimit);
    }

    const ComponentTypeRecord* ComponentRegistry::find(ComponentTypeId id) const {
        const auto it = m_indexById.find(id);
        if (it != m_indexById.end()) {
            return &m_componentRecords[it->second];
        }
        return nullptr;
    }

    const ComponentTypeRecord* ComponentRegistry::find(ComponentDenseId id) const {
        if (id.value() < m_componentRecords.size()) {
            return &m_componentRecords[id.value()];
        }
        return nullptr;
    }

    std::span<const ComponentTypeRecord> ComponentRegistry::records() const {
        return m_componentRecords;
    }

    void ComponentRegistry::freeze() {
        m_frozen = true;
    }

    bool ComponentRegistry::frozen() const {
        return m_frozen;
    }

    std::string_view ComponentRegistry::tagOf(const ComponentTypeId id) const {
        const auto it = m_indexById.find(id);
        if (it != m_indexById.end()) {
            return m_componentRecords[it->second].tag;
        }
        return {};
    }

    std::size_t ComponentRegistry::typeCount() const {
        return m_componentRecords.size();
    }

    ComponentTypeId ComponentRegistry::registerType(const std::string_view tag, const ComponentTypeId existingTypeId) {
        TE_CHECK(!m_frozen, "Component registration is closed - {0} is registered after the registry was frozen", tag);
        TE_CHECK(!tag.empty(), "A component tag must not be empty");

        const StringId stringId{tag};
        TE_CHECK(stringId.value() != 0, "Component tag hashes to the invalid id: {0}", tag);

        const ComponentTypeId typeId{stringId};
        TE_CHECK(!existingTypeId.valid() || existingTypeId == typeId, "Component type is already registered under a different tag: {0}", tag);
        TE_CHECK(!m_indexById.contains(typeId), "Component tag already registered: {0}", tag);

        TE_CHECK(m_componentRecords.size() < m_typeLimit, "Component type limit exceeded: {0} is registered after {1} types", tag, m_typeLimit);

        const auto recordIndex = static_cast<std::uint16_t>(m_componentRecords.size());
        const ComponentDenseId denseId{recordIndex};

        m_componentRecords.emplace_back(typeId, denseId, std::string{tag});
        m_indexById.emplace(typeId, recordIndex);
        return typeId;
    }
}
