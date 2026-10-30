#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/systems/ScheduleAccess.hpp>

#include <algorithm>

namespace TechEngine {

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
        return hasBit(m_readMask, denseId) || writes(denseId);
    }

    bool ScheduleAccess::writes(const ComponentDenseId denseId) const {
        return hasBit(m_writeMask, denseId);
    }

    bool ScheduleAccess::touches(const ComponentDenseId denseId) const {
        return reads(denseId) || writes(denseId);
    }

    ScheduleAccess::ScheduleAccess(const ComponentRegistry& registry, std::span<const ComponentTypeId> written, std::span<const ComponentTypeId> readOnly) {
        for (const ComponentTypeId type: written) {
            setBit(m_writeMask, registry.getDenseId(type));
        }
        for (const ComponentTypeId type: readOnly) {
            setBit(m_readMask, registry.getDenseId(type));
        }
    }

    void ScheduleAccess::merge(const ScheduleAccess& added) {
        for (std::size_t i = 0; i < added.m_writeMask.size(); i++) {
            orWord(m_writeMask, i, added.m_writeMask[i]);
        }
        for (std::size_t i = 0; i < added.m_readMask.size(); i++) {
            orWord(m_readMask, i, added.m_readMask[i]);
        }
    }

    bool ScheduleAccess::hasBit(const std::vector<std::uint64_t>& mask, const ComponentDenseId denseId) const {
        const std::size_t wordIndex = denseId.value() / 64;
        return wordIndex < mask.size() && (mask[wordIndex] & (1ULL << (denseId.value() % 64))) != 0;
    }

    void ScheduleAccess::setBit(std::vector<std::uint64_t>& mask, const ComponentDenseId denseId) {
        orWord(mask, denseId.value() / 64, 1ULL << (denseId.value() % 64));
    }

    void ScheduleAccess::orWord(std::vector<std::uint64_t>& mask, const std::size_t wordIndex, const std::uint64_t bits) {
        if (wordIndex >= mask.size()) {
            mask.resize(wordIndex + 1, 0);
        }
        mask[wordIndex] |= bits;
    }
}
