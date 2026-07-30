#pragma once

#include <TechEngine/core/FrameContext.hpp>

namespace TechEngine {
    class FrameLoop {
    public:
        static constexpr double FIXED_DELTA_TIME = 1.0 / 60.0;
        static constexpr double MAX_FRAME_DELTA_TIME = 0.25;

        explicit FrameLoop(Role role, double fixedDeltaTime = FIXED_DELTA_TIME, double maxFrameDeltaTime = MAX_FRAME_DELTA_TIME);

        const FrameContext& advance(double frameDeltaTime);

        const FrameContext& frame() const;

        double accumulator() const;

    private:
        double m_fixedDeltaTime;
        double m_maxFrameDeltaTime;
        double m_accumulator{0.0};
        FrameContext m_frame{};
    };
}
