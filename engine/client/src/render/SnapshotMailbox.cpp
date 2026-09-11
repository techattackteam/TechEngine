#include <render/SnapshotMailbox.hpp>

namespace TechEngine {
    void SnapshotMailbox::publish(const RenderSnapshot& command) {
        const std::lock_guard lock{m_mutex};
        m_command = command;
    }

    std::optional<RenderSnapshot> SnapshotMailbox::snapshot() const {
        const std::lock_guard lock{m_mutex};
        return m_command;
    }

    void SnapshotMailbox::reset() {
        const std::lock_guard lock{m_mutex};
        m_command.reset();
    }
}
