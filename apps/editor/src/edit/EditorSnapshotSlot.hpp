#pragma once

#include <edit/EditorSnapshot.hpp>

#include <mutex>

namespace TechEngine {
    class EditorSnapshotSlot {
    private:
        std::mutex m_mutex;
        EditorSnapshot m_snapshot;
        bool m_hasNew = false;

    public:
        void publish(EditorSnapshot snapshot);

        // Leaves out unchanged when nothing was published since the last take.
        bool take(EditorSnapshot& out);
    };
}
