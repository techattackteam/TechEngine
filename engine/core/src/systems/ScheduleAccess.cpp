#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

namespace TechEngine {
    ScheduleAccess::ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly) {
        for (const ComponentTypeId type: written) {
            const ComponentDenseId dense = registry.denseId(type);
            const std::size_t index = dense.value() / 64;
            const std::size_t bit = dense.value() % 64;
            if (index >= m_writeMask.size()) {
                m_writeMask.resize(index + 1, 0);
            }
            m_writeMask[index] |= (1ULL << bit);
        }
        for (const ComponentTypeId type: readOnly) {
            const ComponentDenseId dense = registry.denseId(type);
            const std::size_t index = dense.value() / 64;
            const std::size_t bit = dense.value() % 64;
            if (index >= m_readMask.size()) {
                m_readMask.resize(index + 1, 0);
            }
            m_readMask[index] |= (1ULL << bit);
        }
    }

    bool ScheduleAccess::reads(const ComponentDenseId denseId) const {
        const std::size_t index = denseId.value() / 64;
        const std::size_t bit = denseId.value() % 64;
        if (index < m_readMask.size() && (m_readMask[index] & (1ULL << bit)) != 0) {
            return true;
        }
        return writes(denseId);
    }

    bool ScheduleAccess::writes(const ComponentDenseId denseId) const {
        const std::size_t index = denseId.value() / 64;
        const std::size_t bit = denseId.value() % 64;
        if (index >= m_writeMask.size()) {
            return false;
        }
        return (m_writeMask[index] & (1ULL << bit)) != 0;
    }

    bool ScheduleAccess::touches(const ComponentDenseId denseId) const {
        return reads(denseId) || writes(denseId);
    }
}
