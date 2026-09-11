#include <TechEngine/base/diagnostics/Log.hpp>

#include <render/SnapshotHistory.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace TechEngine {
    void SnapshotHistory::acquire(const RenderSnapshot& snapshot) {
        if (!std::isfinite(snapshot.fixedDeltaTime) || snapshot.fixedDeltaTime <= 0.0) {
            TE_LOGGER_ERROR("Rejected snapshot with invalid fixed interval");
            return;
        }
        if (!m_current || snapshot.timeline > m_current->timeline) {
            m_previous.reset();
            m_current = snapshot;
            return;
        }
        if (snapshot.timeline < m_current->timeline || snapshot.tick <= m_current->tick) {
            return;
        }
        if (snapshot.tickTime <= m_current->tickTime || snapshot.fixedDeltaTime != m_current->fixedDeltaTime) {
            TE_LOGGER_ERROR("Snapshot timeline changed without a new generation");
            m_previous.reset();
        } else {
            m_previous = m_current;
        }
        m_current = snapshot;
    }

    double SnapshotHistory::alpha(Clock::TimePoint now) const {
        if (!m_previous || !m_current) {
            return 1.0;
        }
        const double gap = std::chrono::duration<double>(m_current->tickTime - m_previous->tickTime).count();
        const double elapsed = std::chrono::duration<double>(now - m_previous->tickTime).count() - m_current->fixedDeltaTime;
        return std::clamp(elapsed / gap, 0.0, 1.0);
    }

    RenderSnapshot SnapshotHistory::prepareFrame(Clock::TimePoint now, const InputState& input) const {
        RenderSnapshot result = m_current.value_or(RenderSnapshot{});
        if (m_previous && m_current) {
            const auto blend = static_cast<float>(alpha(now));
            for (std::size_t i = 0; i < result.clearColor.size(); i++) {
                result.clearColor[i] = std::lerp(m_previous->clearColor[i], m_current->clearColor[i], blend);
            }
        }
        if (!m_current || input.focusGeneration > result.input.focusGeneration || (input.focusGeneration == result.input.focusGeneration && input.sequence >= result.input.sequence)) {
            result.input = input;
        }
        return result;
    }
}
