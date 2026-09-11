#pragma once

#include <TechEngine/client/render/RenderSnapshot.hpp>

#include <mutex>
#include <optional>

namespace TechEngine {
    class SnapshotMailbox {
    private:
        mutable std::mutex m_mutex;
        std::optional<RenderSnapshot> m_command;

    public:
        void publish(const RenderSnapshot& command);

        std::optional<RenderSnapshot> snapshot() const;

        void reset();
    };
}
