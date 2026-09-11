#pragma once

#include <TechEngine/client/render/RenderSnapshot.hpp>

#include <optional>

namespace TechEngine {
    class SnapshotHistory {
    private:
        std::optional<RenderSnapshot> m_previous;
        std::optional<RenderSnapshot> m_current;

    public:
        void acquire(const RenderSnapshot& snapshot);

        double alpha(Clock::TimePoint now) const;

        RenderSnapshot prepareFrame(Clock::TimePoint now, const InputState& input) const;
    };
}
