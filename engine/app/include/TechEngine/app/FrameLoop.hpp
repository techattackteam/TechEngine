#pragma once

#include <TechEngine/core/FrameContext.hpp>

namespace TechEngine {
    class FrameLoop {
    public:
        static constexpr double FIXED_DT = 1.0 / 60.0;
        static constexpr double MAX_FRAME_DT = 0.25;

        explicit FrameLoop(Role role, double fixedDt = FIXED_DT, double maxFrameDt = MAX_FRAME_DT);

        const FrameContext& advance(double frameDt);

        const FrameContext& frame() const;

        double accumulator() const;

    private:
        double m_fixedDt;
        double m_maxFrameDt;
        double m_accumulator{0.0};
        FrameContext m_frame{};
    };
}
