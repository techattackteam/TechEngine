#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class AOPass : public IRenderPass {
    public:
        const char* name() const override {
            return "AOPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
