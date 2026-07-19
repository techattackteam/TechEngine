#pragma once

#include "renderer/graph/IRenderPass.hpp"

namespace TechEngine {
    class FogPass : public IRenderPass {
    public:
        const char* name() const override {
            return "FogPass";
        }

        void execute(const FrameContext& frame, RenderResources& resources) override;
    };
}
