#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class GodRayPass : public IRenderPass {
    public:
        const char* name() const override {
            return "GodRayPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
