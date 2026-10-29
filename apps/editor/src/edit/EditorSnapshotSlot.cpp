#include <edit/EditorSnapshotSlot.hpp>

#include <utility>

namespace TechEngine {
    void EditorSnapshotSlot::publish(EditorSnapshot snapshot) {
        const std::lock_guard lock{m_mutex};
        m_snapshot = std::move(snapshot);
        m_hasNew = true;
    }

    bool EditorSnapshotSlot::take(EditorSnapshot& out) {
        const std::lock_guard lock{m_mutex};
        if (!m_hasNew) {
            return false;
        }
        out = std::move(m_snapshot);
        m_hasNew = false;
        return true;
    }
}
