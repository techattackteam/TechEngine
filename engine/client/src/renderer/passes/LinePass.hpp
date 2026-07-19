#pragma once

#include <vector>

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class Line;

    class LinePass : public IRenderPass {
    private:
        std::vector<Line>& m_lines;

    public:
        explicit LinePass(std::vector<Line>& lines) : m_lines(lines) {
        }

        const char* name() const override {
            return "LinePass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
