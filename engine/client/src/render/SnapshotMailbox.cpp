#include <render/SnapshotMailbox.hpp>

namespace TechEngine {
    void SnapshotMailbox::publish(const RenderSnapshot& command) {
        const std::lock_guard lock{m_mutex};
        m_command = command;
    }

    bool SnapshotMailbox::snapshot(RenderSnapshot& out) const {
        const std::lock_guard lock{m_mutex};
        if (!m_command.has_value()) {
            return false;
        }
        out = *m_command;
        return true;
    }

    void SnapshotMailbox::reset() {
        const std::lock_guard lock{m_mutex};
        m_command.reset();
    }
}
