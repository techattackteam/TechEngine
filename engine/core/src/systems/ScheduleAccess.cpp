#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

#include <algorithm>

namespace TechEngine {
    ScheduleAccess::ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly) {
        for (const ComponentTypeId type: written) {
            const ComponentDenseId dense = registry.denseId(type);
            const std::size_t index = dense.value() / 64;
            const std::size_t bit = dense.value() % 64;
            if (index >= m_writeMask.size()) {
                m_writeMask.resize(index + 1, 0);
            }
            if ((m_writeMask[index] & (1ULL << bit)) == 0) {
                m_writtenTypes.push_back(dense);
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

    bool ScheduleAccess::conflicts(const ScheduleAccess& other) const {
        const std::size_t wordCount = std::max({m_readMask.size(), m_writeMask.size(), other.m_readMask.size(), other.m_writeMask.size()});
        for (std::size_t i = 0; i < wordCount; i++) {
            const std::uint64_t reads = i < m_readMask.size() ? m_readMask[i] : 0;
            const std::uint64_t writes = i < m_writeMask.size() ? m_writeMask[i] : 0;
            const std::uint64_t otherReads = i < other.m_readMask.size() ? other.m_readMask[i] : 0;
            const std::uint64_t otherWrites = i < other.m_writeMask.size() ? other.m_writeMask[i] : 0;
            if ((writes & (otherReads | otherWrites)) != 0 || (otherWrites & (reads | writes)) != 0) {
                return true;
            }
        }
        return false;
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

    std::span<const ComponentDenseId> ScheduleAccess::getWrittenTypes() const {
        return m_writtenTypes;
    }
}
